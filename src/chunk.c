#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "vm.h"
#include <stddef.h>

/// Initialize a Chunk structure.
void init_chunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  init_value_array(&chunk->constants);
}
/// Write a byte and its corresponding line number to a Chunk.
void write_chunk(Chunk *chunk, uint8_t byte, size_t line) {
  if (chunk->capacity < chunk->count + 1) {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    chunk->lines =
        GROW_ARRAY(size_t, chunk->lines, old_capacity, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

/// Free the memory allocated for a Chunk.
void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(size_t, chunk->lines, chunk->capacity);
  free_value_array(&chunk->constants);
  init_chunk(chunk);
}

/// Add a constant value to a Chunk and return its index.
size_t add_constant(Chunk *chunk, Value value) {
  push(value);
  write_value_array(&chunk->constants, value);
  pop();
  return chunk->constants.count - 1;
}
