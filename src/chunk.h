#pragma once
#include "value.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { OP_CONSTANT, OP_NIL, OP_TRUE, OP_FALSE, OP_RETURN } OpCode;

typedef struct {
  size_t count;
  size_t capacity;
  uint8_t *code;
  size_t *lines;
  ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, size_t line);
void free_chunk(Chunk *chunk);
size_t add_constant(Chunk *chunk, Value value);
