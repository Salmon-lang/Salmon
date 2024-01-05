#include "object.h"
#include "chunk.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_OBJ(type, object_type)                                        \
  (type *)allocate_object(sizeof(type), object_type)

static Obj *allocate_object(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocat %zu for %d\n", (void *)object, size, type);
#endif /* ifdef DEBUG_LOG_GC */

  return object;
}

ObjBoundMethod *new_bound_method(Value reciever, ObjClosure *method) {
  ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
  bound->reciever = reciever;
  bound->method = method;
  return bound;
}

ObjClass *new_class(ObjString *name) {
  ObjClass *class = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  class->name = name;
  init_table(&class->methods);
  return class;
}

ObjClosure *new_closure(ObjFunction *function) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalue_count);
  for (size_t i = 0; i < function->upvalue_count; ++i) {
    upvalues[i] = NULL;
  }
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

ObjFunction *new_function() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

ObjInstance *new_instance(ObjClass *klass) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  init_table(&instance->fields);
  return instance;
}

ObjNative *new_native(NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjArray *new_array() {
  ObjArray *array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
  init_value_array(&array->values);
  return array;
}

static char *format(char *chars) {
  size_t max_length = strlen(chars);
  char *formated = malloc(max_length + 1);
  for (size_t i = 0, j = 0; i < max_length; ++i, ++j) {
    if (chars[i] == '\\' && i < max_length - 1) {
      switch (chars[++i]) {
      case 'n':
        formated[j] = '\n';
        break;
      case 't':
        formated[j] = '\t';
        break;
      case 'r':
        formated[j] = '\r';
      case '\\':
        formated[j] = '\\';
        break;
      case '\"':
        formated[j] = '\"';
        break;
      default:
        i--;
        break;
      }
      continue;
    }
    formated[j] = chars[i];
  }
  return formated;
}

static ObjString *allocate_string(char *chars, size_t length, uint32_t hash,
                                  bool string_literal) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = string_literal ? format(chars) : chars;
  string->hash = hash;
  push(OBJ_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  pop();
  return string;
}

static uint32_t hash_string(const char *key, size_t length) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < length; ++i) {
    hash ^= (uint32_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString *take_string(char *chars, size_t length) {
  uint32_t hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return allocate_string(chars, length, hash, true);
}

ObjString *copy_string(const char *chars, size_t length, bool strlit) {
  uint32_t hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length, hash, strlit);
}

ObjUpvalue *new_upvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->location = slot;
  upvalue->next = NULL;
  upvalue->closed = NIL_VAL;
  return upvalue;
}

static void print_function(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

static void print_array(ObjArray *array) {
  printf("[");
  for (size_t i = 0; i < array->values.count; ++i) {
    print_value(array->values.value[i]);
    if (i < array->values.count - 1) {
      printf(", ");
    }
  }
  printf("]");
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_ARRAY:
    print_array(AS_ARRAY(value));
    break;
  case OBJ_BOUND_METHOD:
    print_function(AS_BOUND_METHOD(value)->method->function);
    break;
  case OBJ_CLASS:
    printf("%s", AS_CLASS(value)->name->chars);
    break;
  case OBJ_CLOSURE:
    print_function(AS_CLOSURE(value)->function);
    break;
  case OBJ_FUNCTION:
    print_function(AS_FUNCTION(value));
    break;
  case OBJ_INSTANCE: {
    printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
    break;
  }
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  }
}
