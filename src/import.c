#include "import.h"
#include "stdlib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *start;
  const char *current;
} ImportScanner;

static char *read_file(const char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", file_path);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enought memory to read \"%s\".\n", file_path);
    exit(74);
  }
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file \"%s\".\n", file_path);
    exit(74);
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}
// Utility functions for character handling
static char peek(const char *start) { return *start; }

static char peek_next(const char *start) {
  return *start == '\0' ? '\0' : start[1];
}

static void advance(const char **start) { (*start)++; }

// Function to skip whitespace
static const char *skip_white_space(const char *start) {
    while (*start == ' ' || *start == '\r' || *start == '\t' || *start == '\n') {
        advance(&start);
    }
    return start;
}

// Function to calculate the length of a substring without whitespace
static size_t length_no_whitespace(const char *start, const char *end) {
  size_t length = 0;
  while (start <= end) {
    start = skip_white_space(start);
    if (start <= end) {
      length++;
      start++;
    }
  }
  return length;
}

// Function to check if a given substring is an import statement
static bool is_import(const char *start_of_file, const char *start) {
  start = skip_white_space(start);
  if (*start == 'i' && *(start + 1) == 'm' && *(start + 2) == 'p' &&
      *(start + 3) == 'o' && *(start + 4) == 'r' && *(start + 5) == 't') {
    return true;
  }
  return false;
}

// Function to extract imports from a substring
static char *grab_imports(const char *start) {
  const char *end = strchr(start, '}');
  if (end == NULL) {
    return NULL;
  }

  size_t length = length_no_whitespace(start, end);
  char *imports = (char *)malloc(length + 1);

  const char *temp = start;
  for (size_t i = 0; i < length; ++i) {
    temp = skip_white_space(temp);
    imports[i] = *temp;
    temp++;
  }
  imports[length] = '\0';

  return imports;
}

// Main function to combine files and extract imports
char *combine_files(const char *file_path) {
  char *file_contents = read_file(file_path);
  const char *start = strchr(file_contents, 'i');

  if (start != NULL && is_import(file_contents, start)) {
    start += 6;
    start = skip_white_space(start);
    printf("%s", start);
    switch (*(start)) {
    case '{': {
      char *imports = grab_imports(start);
      printf("%s", imports);
      free(imports);
      break;
    }
    default:
      printf("Import must have a body.\n");
      return NULL;
    }
  }

  return file_contents;
}
