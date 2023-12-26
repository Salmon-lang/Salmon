#pragma once
#include <string.h>
#include <stdbool.h>

typedef enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER } ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

typedef struct {
  size_t capacity;
  size_t count;
  Value *value;
} ValueArray;
bool values_equal(Value a, Value b);
void init_value_array(ValueArray *arr);
void write_value_array(ValueArray *arr, Value val);
void free_value_array(ValueArray *arr);
void print_value(Value value);
