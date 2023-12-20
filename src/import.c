#include "import.h"
#include "memory.h"
#include "stdlib.h"
#ifdef __linux__
#include <linux/limits.h>
#elif __APPLE__
#include <sys/syslimits.h>
#elif _WIN32
#include <windows.h>
#define PATH_MAX MAX_PATH
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  size_t length;
  size_t capacity;
  size_t *import_count;
  char **contents;
  char **imports;
} Imports;

static void init_imports(Imports *imports) {
  imports->length = 0;
  imports->capacity = 0;
  imports->import_count = NULL;
  imports->contents = NULL;
  imports->imports = NULL;
}

static void free_imports(Imports *imports) {
  FREE_ARRAY(size_t, imports->import_count, imports->capacity);
  FREE_ARRAY(char *, imports->imports, imports->capacity);
  FREE_ARRAY(char *, imports->contents, imports->capacity);
  init_imports(imports);
}

static bool contains_import(char **imports, size_t length, char *import) {
  for (size_t i = 0; i < length; ++i) {
    if (strcmp(imports[i], import) == 0) {
      return true;
    }
  }
  return false;
}

static size_t get_index_of_import(char **imports, size_t length, char *import) {
  for (size_t i = 0; i < length; ++i) {
    if (strcmp(imports[i], import) == 0) {
      return i;
    }
  }
  return 0;
}

static void write_import(Imports *imports, char *import, char *contents) {
  if (contains_import(imports->imports, imports->length, import)) {
    imports->import_count[get_index_of_import(imports->imports, imports->length,
                                              import)]++;
    return;
  }
  if (imports->capacity < imports->length + 1) {
    size_t old_capacity = imports->capacity;
    imports->capacity = GROW_CAPACITY(old_capacity);
    imports->imports =
        GROW_ARRAY(char *, imports->imports, old_capacity, imports->capacity);
    imports->import_count = GROW_ARRAY(size_t, imports->import_count,
                                       old_capacity, imports->capacity);
    imports->contents =
        GROW_ARRAY(char *, imports->contents, old_capacity, imports->capacity);
  }
  imports->imports[imports->length] = import;
  imports->import_count[imports->length] = 1;
  imports->contents[imports->length] = contents;
  imports->length++;
}

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
static const char *skip_whitespace(const char *start) {
  while (*start == ' ' || *start == '\r' || *start == '\t' || *start == '\n') {
    advance(&start);
  }
  return start;
}

// Function to calculate the length of a substring without whitespace
static size_t length_no_whitespace(const char *start, const char *end) {
  size_t length = 0;
  while (start <= end) {
    start = skip_whitespace(start);
    if (start <= end) {
      length++;
      start++;
    }
  }
  return length;
}

// Function to check if a given substring is an import statement
static bool is_import(const char *start_of_file, const char *start) {
  start = skip_whitespace(start);
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
  char *imports = (char *)malloc(length);

  const char *temp = start;
  for (size_t i = 0; i < length - 1; ++i) {
    temp = skip_whitespace(temp);
    imports[i] = *temp;
    temp++;
  }
  imports[length - 1] = '\0';

  return imports;
}

static char *get_abs_file_path(char *relative_path) {
  printf("%s\n", relative_path);
  char actual_path[PATH_MAX];
#ifdef __unix__

  char *result = realpath(strcat(relative_path, ".salmon"), actual_path);
  if (result) {
    printf("Absolute Path: %s\n", actual_path);
  } else {
    perror("realpath");
    // Handle the error, if any
  }
  return result;
#elif _WIN32
  DWORD result = GetFullPathNameA(strcat(relative_path, ".salmon"), PATH_MAX,
                                  actual_path, NULL);
  if (result != 0) {
    printf("Absolute Path: %s\n", resolved_path);
  } else {
    perror("GetFullPathName");
    // Handle the error, if any
  }
  char buffer[PATH_MAX];
  sprintf(buffer, "%lu", result);
  return buffer;
#endif
}

char **split_string(const char *input, const char *delimiter,
                    size_t *arraySize) {
  printf("%s\n", input);
  char *str = strdup(
      input); // Duplicate the input string to avoid modifying the original
  char *token = strtok(str, delimiter);

  *arraySize = 0;

  // Count the number of tokens
  while (token != NULL) {
    (*arraySize)++;
    token = strtok(NULL, delimiter);
  }

  // Allocate memory for the array of char pointers
  char **resultArray = (char **)malloc(sizeof(char *) * (*arraySize));

  // Reset the string for tokenization
  strcpy(str, input);

  // Tokenize the input string and store pointers in the array
  token = strtok(str, delimiter);
  for (size_t i = 0; i < *arraySize; i++) {
    resultArray[i] = strdup(token);
    token = strtok(NULL, delimiter);
  }

  free(str); // Free the duplicated string
  return resultArray;
}

static void print_imports(Imports *imports) {
  for (size_t i = 0; i < imports->length; ++i) {
    printf("%s: %zu, %s\n", imports->imports[i], imports->import_count[i],
           imports->contents[i]);
  }
}

static char *remove_suffix(char *str, const char *suffix) {
  size_t str_length = strlen(str);
  size_t suffix_length = strlen(suffix);

  if (str_length >= suffix_length &&
      strcmp(str + (str_length - suffix_length), suffix) == 0) {
    str[str_length - suffix_length] = '\0';
  }
  return str;
}

static char *get_content(char *file_path) { return ""; }

// Main function to combine files and extract imports
char *combine_files(char *file_path) {
  char *path = malloc(strlen(file_path) + 1);
  strcpy(path, file_path);
  char *file_contents = read_file(file_path);
  const char *start = strchr(file_contents, 'i');

  if (start != NULL && is_import(file_contents, start)) {
    start += 6;
    start = skip_whitespace(start); // Corrected function name
    printf("%s", start);

    switch (*start) {
    case '{': {
      char *c = malloc(strlen(start) + 1); // Corrected memory allocation
      strcpy(c, start);
      char *imports = grab_imports(c + 1);
      printf("%s\n", imports);

      size_t length = 0;
      char delimiter[] = ",";
      char *comma = ",";
      // Assuming split_string allocates memory for each element in the array
      char **import_list = split_string(imports, delimiter, &length);

      printf("Array Size: %zu\n", length);
      for (size_t i = 0; i < length; ++i) {
        printf("%zu: %s\n", i, import_list[i]);
      }

      Imports im;
      printf("im\n");
      init_imports(&im);
      printf("init\n");
      write_import(&im, get_abs_file_path(remove_suffix(path, ".salmon")),
                   get_content(file_path));
      for (size_t i = 0; i < length; ++i) {
        printf("%zu\n", i);
        char *current_import = get_abs_file_path(import_list[i]);
        write_import(&im, current_import,
                     contains_import(im.imports, im.length, current_import)
                         ? ""
                         : get_content(current_import));
      }
      print_imports(&im);
      free_imports(&im);

      // Free allocated memory
      free(c);
      break;
    }
    default:
      printf("Import must have a body.\n");
      return NULL;
    }
  }
  return file_contents;
}
