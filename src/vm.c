#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

VM vm;
/// Native function for finding the length of an array/string.
///
/// Parameters:
///   arg_count: The number of arguments (should be 1).
///   args: An array of values representing the arguments passed to the native
///   function.
///
/// Returns:
///   A new value representing the length of the array/string, otherwise nil.
static Value length_native(size_t arg_count, Value *args) {
  if (IS_ARRAY(args[0])) {
    return NUMBER_VAL((double)(AS_ARRAY(args[0])->values.count));
  } else if (IS_STRING(args[0])) {
    return NUMBER_VAL((double)(AS_STRING(args[0])->length));
  } else {
    return NIL_VAL;
  }
}
/// Native function for retrieving the current time in seconds.
///
/// Parameters:
///   arg_count: The number of arguments (should be 0).
///   args: An array of values representing the arguments (not used).
///
/// Returns:
///   A new value representing the current time in seconds.
static Value clock_native(size_t arg_count, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
/// Prints the native function's result to the standard output.
///
/// Parameters:
///   arg_count: The number of arguments passed to the native function.
///   args: An array of values representing the arguments passed to the native
///   function.
///
/// Returns:
///   The result value of the native function.
static Value print_native(size_t arg_count, Value *args) {
  print_value(args[0]);
  return NIL_VAL;
}
/// Resets the VM's stack, frame count, and open upvalues.
static void reset_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
  vm.open_upvalues = NULL;
}
/// Handles runtime errors, printing an error message with formatted output to
/// stderr. Also displays file, line, and function information for the error.
///
/// Parameters:
///   format: The format string for the error message.
///   ...: Variable arguments for the format string.
static void runtime_error(const char *format, ...) {
  // prints an error message with formated output to stderr.
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  // Displays file, line, and function information for the error.
  for (ssize_t i = vm.frame_count - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[file %s, line %zu] in ", vm.path,
            function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_stack();
}
/// Defines a native function in the VM's global table.
///
/// Parameters:
///   name: The name of the native function.
///   function: A pointer to the native function.
static void define_native(const char *name, NativeFn function) {
  // creates a string object representing the native function name.
  push(OBJ_VAL(copy_string(name, strlen(name), false)));
  // creates a closure for the native function and adds it to the global table.
  push(OBJ_VAL(new_native(function)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
/// Initializes the virtual machine, resetting the stack and initializing
/// various VM components.
void init_VM() {
  reset_stack();
  vm.objects = NULL;
  vm.bytes_allocated = 0;
  vm.next_gc = 1024 * 1024;
  vm.gray_capacity = 0;
  vm.gray_count = 0;
  vm.gray_stack = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);

  vm.init_string = NULL;
  vm.init_string = copy_string("init", 4, false);

  define_native("_length", length_native);
  define_native("_clock", clock_native);
  define_native("_print", print_native);
}

/// Frees the resources associated with the virtual machine, including global
/// and string tables.
void free_VM() {
  free_table(&vm.globals);
  free_table(&vm.strings);
  vm.init_string = NULL;
  free_objects();
}
/// Pushes a value onto the VM's stack.
///
/// Parameters:
///   value: The value to be pushed onto the stack.
void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}
/// Pops a value from the VM's stack.
///
/// Returns:
///   The popped value.
Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}
/// Peeks at the value on the stack at a given distance from the top.
///
/// Parameters:
///   distance: The distance from the top of the stack (0 for the top).
///
/// Returns:
///   The value at the specified distance from the top of the stack.
static Value peek(int distance) { return vm.stack_top[-1 - distance]; }
/// Calls a closure with the specified number of arguments.
///
/// Parameters:
///   closure: The closure to be called.
///   arg_count: The number of arguments to pass to the closure.
///
/// Returns:
///   A boolean indicating whether the call was successful.
///   If false, a runtime error occurred.
static bool call(ObjClosure *closure, size_t arg_count) {
  if (arg_count != closure->function->arity) {
    runtime_error("Expected %d arguments but got %d.", closure->function->arity,
                  arg_count);
    return false;
  }
  // check for stack overflow
  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }
  // creates a call frame for the closure
  CallFrame *frame = &vm.frames[vm.frame_count++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - arg_count - 1;
  return true;
}
/// Calls a value as a function with the specified number of arguments.
///
/// Parameters:
///   callee: The value to be called.
///   arg_count: The number of arguments to pass to the callee.
///
/// Returns:
///   A boolean indicating whether the call was successful.
///   If false, a runtime error occurred.
static bool call_value(Value callee, size_t arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
      vm.stack_top[-arg_count - 1] = bound->reciever;
      return call(bound->method, arg_count);
    }
    case OBJ_CLASS: {
      ObjClass *class = AS_CLASS(callee);
      vm.stack_top[-arg_count - 1] = OBJ_VAL(new_instance(class));
      Value initializer;
      if (table_get(&class->methods, vm.init_string, &initializer)) {
        return call(AS_CLOSURE(initializer), arg_count);
      } else if (arg_count != 0) {
        runtime_error("Expected 0 arguments but got %d.", arg_count);
        return false;
      }
      return true;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), arg_count);
    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(arg_count, vm.stack_top - arg_count);
      vm.stack_top -= arg_count + 1;
      push(result);
      return true;
    }
    default:
      break;
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}
/// Invokes a method on a class with the specified method name and number of
/// arguments.
///
/// Parameters:
///   class: The class on which the method is invoked.
///   name: The name of the method to invoke.
///   arg_count: The number of arguments to pass to the method.
///
/// Returns:
///   A boolean indicating whether the invocation was successful.
///   If false, a runtime error occurred.
static bool invoke_from_class(ObjClass *class, ObjString *name,
                              size_t arg_count) {
  Value method;
  if (!table_get(&class->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), arg_count);
}
/// Invokes a method on an object or class with the specified number of
/// arguments.
///
/// Parameters:
///   name: The name of the method to invoke.
///   arg_count: The number of arguments to pass to the method.
///
/// Returns:
///   A boolean indicating whether the invocation was successful.
///   If false, a runtime error occurred.
static bool invoke(ObjString *name, size_t arg_count) {
  Value reciever = peek(arg_count);
  if (!IS_INSTANCE(reciever)) {
    runtime_error("Only instances have methods.");
    return false;
  }
  ObjInstance *instance = AS_INSTANCE(reciever);
  Value value;
  // tries to acces the meathod from the instance's fields.
  if (table_get(&instance->fields, name, &value)) {
    vm.stack_top[-arg_count - 1] = value;
    return call_value(value, arg_count);
  }
  // if not fount in the instance, looks for the method in its class
  return invoke_from_class(instance->klass, name, arg_count);
}
/// Binds a method to an object instance by creating a bound method.
///
/// Parameters:
///   class: The class of the object instance.
///   name: The name of the method to bind.
///
/// Returns:
///   A boolean indicating whether the method binding was successful.
///   If false, a runtime error occurred.
static bool bind_method(ObjClass *class, ObjString *name) {
  Value method;
  if (!table_get(&class->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  ObjBoundMethod *bound = new_bound_method(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}
/// Captures the value of a local variable for use as an upvalue.
///
/// Parameters:
///   local: A pointer to the local variable to be captured.
///
/// Returns:
///   A pointer to the captured upvalue.
///   If the upvalue already exists, the existing upvalue is returned.
///   If a new upvalue is created, it is added to the list of open upvalues.
static ObjUpvalue *capture_upvalue(Value *local) {
  ObjUpvalue *prev_upvalue = NULL;
  ObjUpvalue *upvalue = vm.open_upvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next;
  }
  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }
  ObjUpvalue *created_upvalue = new_upvalue(local);
  created_upvalue->next = upvalue;
  if (prev_upvalue == NULL) {
    vm.open_upvalues = created_upvalue;
  } else {
    prev_upvalue->next = created_upvalue;
  }
  return created_upvalue;
}
/// Closes the upvalues that are no longer in scope.
///
/// Parameters:
///   last: A pointer to the top of the stack, indicating the last value still
///   in scope.
static void close_upvalue(Value *last) {
  while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
    ObjUpvalue *upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}
/// Defines a method for a class by adding it to the class's method table.
///
/// Parameters:
///   name: The name of the method.
static void define_method(ObjString *name) {
  Value method = peek(0);
  ObjClass *class = AS_CLASS(peek(1));
  table_set(&class->methods, name, method);
  pop();
}
/// Checks if a given value is "falsey" according to Lox rules (in Salmon 0 is
/// false).
///
/// Parameters:
///   value: The value to check for truthiness or falsiness.
///
/// Returns:
///   A boolean indicating whether the value is falsey.
static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_NUMBER(value) && AS_NUMBER(value) == 0.0) ||
         (IS_BOOL(value) && !AS_BOOL(value));
}
/// Appends the top value of the stack to the array on the stack.
static void append() {
  ObjArray array;
  array = *AS_ARRAY(peek(1));
  Value val = peek(0);
  write_value_array(&array.values, val);
  pop();
  pop();
  ObjArray *arr = new_array();
  arr->values = array.values;
  push(OBJ_VAL(arr));
}
/// Concatenates two strings at the top of the stack.
static void concatonate() {
  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));
  size_t length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';
  ObjString *result = take_string(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

/// Executes the bytecode in the current call frame.
///
/// This function interprets the bytecode instructions and executes the
/// corresponding operations. It maintains the call frame and stack to keep
/// track of the program state during execution.
///
/// Returns:
///   The interpretation result, indicating success or the type of error
///   encountered.
static InterpretResult run() {
  CallFrame *frame = &vm.frames[vm.frame_count - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2]) << 8 | frame->ip[-1]))
#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.value[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(value_type(a op b));                                                  \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(
        &frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
#endif /* ifdef DEBUG_TRACE_EXECUTION */
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_PATH: {
      if (IS_STRING(peek(0))) {
        ObjString *path = AS_STRING(pop());
        vm.path = path->chars;
      }
      break;
    }
    case OP_NIL:
      push(NIL_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_POP:
      pop();
      break;
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      table_set(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        table_delete(&vm.globals, name);
        runtime_error("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }
    case OP_GET_PROPERTY: {
      if (!IS_INSTANCE(peek(0))) {
        runtime_error("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();
      Value value;
      if (table_get(&instance->fields, name, &value)) {
        pop();
        push(value);
        break;
      }
      if (!bind_method(instance->klass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SET_PROPERTY: {
      if (!IS_INSTANCE(peek(1))) {
        runtime_error("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(1));
      table_set(&instance->fields, READ_STRING(), peek(0));
      Value value = pop();
      pop();
      push(value);
      break;
    }
    case OP_GET_SUPER: {
      ObjString *name = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());
      if (!bind_method(superclass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_ELEMENT: {
      if (!IS_ARRAY(peek(1)) && !IS_STRING(peek(1))) {
        runtime_error("Can not access element of a non array/string.");
      }
      if (IS_ARRAY(peek(1))) {
        ObjArray *array = AS_ARRAY(peek(1));
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Index must be a number.");
        }
        Value index = peek(0);
        Value value;
        int i = (int)(AS_NUMBER(index));
        if (i < array->values.count && i >= 0) {
          pop();
          pop();
          value = array->values.value[i];
          push(value);
          break;
        }
        runtime_error("Index of %d out of bounds for array of length %zu.", i,
                      array->values.count);
        break;
      } else {
        ObjString *string = AS_STRING(peek(1));
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Index must be a number.");
        }
        Value index = peek(0);
        int i = (int)(AS_NUMBER(index));
        if (i < string->length && i >= 0) {
          pop();
          pop();
          char *c = ALLOCATE(char, 2);
          memcpy(c, string->chars + i, 1);
          c[1] = '\0';
          ObjString *result = take_string(c, 2);
          push(OBJ_VAL(result));
          break;
        }
        runtime_error("Index of %d out of bounds for array of length %zu.", i,
                      string->length);
        break;
      }
      break;
    }
    case OP_SET_ELEMENT: {
      if (!IS_ARRAY(peek(2))) {
        runtime_error("Cannot set element of a non-array.");
      }

      ObjArray *originalArray = AS_ARRAY(peek(2));

      if (!IS_NUMBER(peek(1))) {
        runtime_error("Index must be a number.");
      }

      Value index = peek(1);
      int i = (int)(AS_NUMBER(index));

      if (i < originalArray->values.count && i >= 0) {
        Value value = pop(); // Pop the value to be set.

        ObjArray *modifiedArray = new_array(); // Create a new array.

        // Copy the values from the original array to the modified array.
        for (size_t j = 0; j < originalArray->values.count; j++) {
          if (j == (size_t)i) {
            write_value_array(&modifiedArray->values,
                              value); // Set the new value.
          } else {
            write_value_array(&modifiedArray->values,
                              originalArray->values.value[j]);
          }
        }

        pop();               // Pop the index.
        pop();               // Pop the array.
        push(value);                  // Push the value back onto the stack.
        push(OBJ_VAL(modifiedArray)); // Push the modified array onto the stack.
        break;
      }

      runtime_error("Index of %d out of bounds for array of length %zu.", i,
                    originalArray->values.count);
      break;
    }
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }
    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
      BINARY_OP(BOOL_VAL, <);
      break;
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatonate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else if (IS_ARRAY(peek(1))) {
        append();
      } else {
        runtime_error("Operands must be either two strings or two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;
    case OP_NOT:
      push(BOOL_VAL(is_falsey(pop())));
      break;
    case OP_NEGATE:
      if (!IS_NUMBER(peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(0))) {
        frame->ip += offset;
      }
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }
    case OP_CALL: {
      size_t arg_count = READ_BYTE();
      if (!call_value(peek(arg_count), arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    case OP_INVOKE: {
      ObjString *method = READ_STRING();
      size_t arg_count = READ_BYTE();
      if (!invoke(method, arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    case OP_SUPER_INVOKE: {
      ObjString *method = READ_STRING();
      size_t arg_count = READ_BYTE();
      ObjClass *superclass = AS_CLASS(pop());
      if (!invoke_from_class(superclass, method, arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = new_closure(function);
      push(OBJ_VAL(closure));
      for (size_t i = 0; i < closure->upvalue_count; ++i) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (is_local) {
          closure->upvalues[i] = capture_upvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE:
      close_upvalue(vm.stack_top - 1);
      pop();
      break;
    case OP_RETURN: {
      Value result = pop();
      close_upvalue(frame->slots);
      vm.frame_count--;
      if (vm.frame_count == 0) {
        pop();
        return INTERPRET_OK;
      }
      vm.stack_top = frame->slots;
      push(result);
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    case OP_CLASS:
      push(OBJ_VAL(new_class(READ_STRING())));
      break;
    case OP_INHERIT: {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass)) {
        runtime_error("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjClass *subclass = AS_CLASS(peek(0));
      table_add_all(&AS_CLASS(superclass)->methods, &subclass->methods);
      pop();
      break;
    }
    case OP_METHOD:
      define_method(READ_STRING());
      break;
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    }
  }

#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}
/// Interprets the source code provided and executes it.
///
/// Parameters:
///   source: The source code to be interpreted.
///
/// Returns:
///   The result of the interpretation (INTERPRET_OK, INTERPRET_COMPILE_ERROR,
///   INTERPRET_RUNTIME_ERROR).
InterpretResult interpret(const char *source) {
  ObjFunction *function = compile(source);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJ_VAL(function));
  ObjClosure *closure = new_closure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}
