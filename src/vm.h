#pragma once

#include "chunk.h"
#include "common.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct CallFrame {
  ObjClosure *closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

typedef struct VM {
  CallFrame frames[FRAMES_MAX];
  size_t frame_count;
  Value stack[STACK_MAX];
  Value *stack_top;
  Table globals;
  Table strings;
  ObjString *init_string;
  ObjUpvalue *open_upvalues;
  size_t bytes_allocated;
  size_t next_gc;
  Obj *objects;
  size_t gray_count;
  size_t gray_capacity;
  Obj **gray_stack;
} VM;

typedef enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();
