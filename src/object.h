#pragma once

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"
#include <stddef.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) is_obj_type(value, OBJ_CLASS)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_ARRAY(value) is_obj_type(value, OBJ_ARRAY)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_ARRAY(value) ((ObjArray *)AS_OBJ(value))

typedef enum ObjType {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_ARRAY,
  OBJ_UPVALUE
} ObjType;

struct Obj {
  ObjType type;
  bool is_marked;
  struct Obj *next;
};

typedef struct ObjFunction {
  Obj obj;
  size_t arity;
  size_t upvalue_count;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(size_t arg_count, Value *args);

typedef struct ObjNative {
  Obj obj;
  NativeFn function;
} ObjNative;

struct ObjString {
  Obj obj;
  size_t length;
  char *chars;
  uint32_t hash;
};

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct ObjClosure {
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  size_t upvalue_count;
} ObjClosure;

typedef struct ObjClass {
  Obj obj;
  ObjString *name;
  Table methods;
} ObjClass;

typedef struct ObjInstance {
  Obj obj;
  ObjClass *klass;
  Table fields;
} ObjInstance;

typedef struct ObjBoundMethod {
  Obj obj;
  Value reciever;
  ObjClosure *method;
} ObjBoundMethod;

typedef struct {
  Obj obj;
  ValueArray values;
} ObjArray;

ObjBoundMethod *new_bound_method(Value reciever, ObjClosure *method);
ObjClass *new_class(ObjString *name);
ObjClosure *new_closure(ObjFunction *function);
ObjFunction *new_function();
ObjInstance *new_instance(ObjClass *klass);
ObjNative *new_native(NativeFn function);
ObjString *take_string(char *chars, size_t length);
ObjString *copy_string(const char *chars, size_t length);
ObjArray *new_array();
ObjUpvalue *new_upvalue(Value *slot);
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
