#include "value.h"
#include "memory.h"
#include <stddef.h>
#include <stdio.h>

bool values_equal(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }
  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  default:
    return false;
  }
}

void init_value_array(ValueArray *arr) {
  arr->value = NULL;
  arr->capacity = 0;
  arr->count = 0;
}

void write_value_array(ValueArray *arr, Value val) {
  if (arr->capacity < arr->count + 1) {
    size_t old_capacity = arr->capacity;
    arr->capacity = GROW_CAPACITY(old_capacity);
    arr->value = GROW_ARRAY(Value, arr->value, old_capacity, arr->capacity);
  }
  arr->value[arr->count] = val;
  arr->count++;
}

void free_value_array(ValueArray *arr) {
  FREE_ARRAY(Value, arr->value, arr->capacity);
  init_value_array(arr);
}

void print_value(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf("%g", AS_NUMBER(value));
    break;
  }
}
