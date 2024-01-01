#include "import.h"
#include "memory.h"
#include "stdlib.h"
#include <ctype.h>

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

/// Structure to manage and store information about imports.
typedef struct {
  size_t length;        ///< Number of imports.
  size_t capacity;      ///< Capacity of the imports array.
  bool *is_imported;    ///< Array to track whether each import is processed.
  size_t *import_count; ///< Array to store the count of each import occurrence.
  char **contents;      ///< Array to store the contents of each import.
  char **imports;       ///< Array to store the names of the imports.
} Imports;

/// Initialize the Imports structure by setting default values and allocating
/// memory.
static void init_imports(Imports *imports) {
  imports->length = 0;
  imports->capacity = 0;
  imports->is_imported = NULL;
  imports->import_count = NULL;
  imports->contents = NULL;
  imports->imports = NULL;
}

/// Free the memory allocated for the Imports structure.
static void free_imports(Imports *imports) {
  FREE_ARRAY(bool, imports->is_imported, imports->capacity);
  FREE_ARRAY(size_t, imports->import_count, imports->capacity);
  FREE_ARRAY(char *, imports->imports, imports->capacity);
  FREE_ARRAY(char *, imports->contents, imports->capacity);
  init_imports(imports);
}

/// Check if list of imports contains import.
static bool contains_import(char **imports, size_t length, char *import) {
  for (size_t i = 0; i < length; ++i) {
    if (strcmp(imports[i], import) == 0) {
      return true;
    }
  }
  return false;
}

/// Find the index of a specific import in the imports array.
static size_t get_index_of_import(char **imports, size_t length, char *import) {
  for (size_t i = 0; i < length; ++i) {
    if (strcmp(imports[i], import) == 0) {
      return i;
    }
  }
  return 0;
}

static size_t get_import_length(char *path);

/// Write an import to the Imports structure, updating counts or adding a new
/// entry.
static void write_import(Imports *imports, char *import, char *contents) {
  for (size_t i = 0; i < imports->length; ++i) {
    if (strcmp(imports->imports[i], import) == 0) {
      imports->import_count[i]++;
      return;
    }
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
    imports->is_imported =
        GROW_ARRAY(bool, imports->is_imported, old_capacity, imports->capacity);
  }
  imports->is_imported[imports->length] = false;
  imports->imports[imports->length] = strdup(import);
  imports->import_count[imports->length] = 1;
  imports->contents[imports->length] = strdup(contents);
  imports->length++;
}

/// Read the contents of a file, allocate memory, and store the file contents.
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
    fprintf(stderr, "Not enough memory to read \"%s\".\n", file_path);
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

/// Peek at the current and next characters in a string without advancing the
/// pointer.
static char peek(const char *start) { return *start; }
static char peek_next(const char *start) {
  return *start == '\0' ? '\0' : start[1];
}

/// Advance the string pointer to the next character.
static void advance(const char **start) { (*start)++; }

/// Skip whitespace characters in a string by moving the pointer past them.
static const char *skip_whitespace(const char *start) {
  while (*start == ' ' || *start == '\r' || *start == '\t' || *start == '\n') {
    advance(&start);
  }
  return start;
}

/// Calculate the length of a substring without whitespace.
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

/// Check if a character is alphanumeric or an underscore.
static bool is_alpha_num_underscore(char c) {
  return isalpha(c) || (c >= '0' && '9' >= c) || c == '_';
}

/// Check if a substring represents an import statement.
static bool is_import(const char *start_of_file, const char *start) {
  start = skip_whitespace(start);

  if (strlen(start) > 7 && *start == 'i' && *(start + 1) == 'm' &&
      *(start + 2) == 'p' && *(start + 3) == 'o' && *(start + 4) == 'r' &&
      *(start + 5) == 't' && !is_alpha_num_underscore(*(start + 6))) {
    return true;
  }
  return false;
}

/// Extract imports from a substring.
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

/// Get the absolute file path given a relative path.
static char *get_abs_file_path(char *relative_path) {
  char input_path[PATH_MAX];
  strcpy(input_path, relative_path);
  char actual_path[PATH_MAX];

#ifdef __unix__
  char *result = realpath(strcat(input_path, ".salmon"), actual_path);
  if (!result) {
    perror("realpath");
    // Handle the error, if any
  }
  return result;
#elif _WIN32
  DWORD result = GetFullPathNameA(strcat(relative_path, ".salmon"), PATH_MAX,
                                  actual_path, NULL);
  if (result != 0) {
    printf("Absolute Path: %s\n", actual_path);
  } else {
    perror("GetFullPathName");
    // Handle the error, if any
  }
  char buffer[PATH_MAX];
  sprintf(buffer, "%lu", result);
  return buffer;
#endif
}

/// Split a string into an array using a delimiter and store the tokens.
char **split_string(const char *input, const char *delimiter,
                    size_t *arraySize) {
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

/// Print the contents of the Imports structure.
static void print_imports(Imports *imports) {
  for (size_t i = 0; i < imports->length; ++i) {
    printf("%s: %zu, %s\n", imports->imports[i], imports->import_count[i],
           imports->contents[i]);
  }
}

/// Remove a specified suffix from a string.
static char *remove_suffix(char *str, const char *suffix) {
  size_t str_length = strlen(str);
  size_t suffix_length = strlen(suffix);

  if (str_length >= suffix_length &&
      strcmp(str + (str_length - suffix_length), suffix) == 0) {
    str[str_length - suffix_length] = '\0';
  }
  return str;
}

/// Get the number of imports in a file by reading its contents and counting.
static size_t get_import_length(char *path) {
  char *contents = read_file(path);

  const char *start = strchr(contents, 'i');

  if (start != NULL && is_import(contents, start)) {
    start += 6;
    start = skip_whitespace(start);

    switch (*start) {
    case '{': {
      char *c = malloc(strlen(start) + 1);
      strcpy(c, start);
      char *imports = grab_imports(c + 1);

      size_t length = 0;
      char delimiter[] = ",";
      char *comma = ",";
      char **import_list = split_string(imports, delimiter, &length);
      free(import_list);
      free(c);
      return length;
    }
    default:
      printf("Import must have a body.\n");
      free(path);
      return 0;
    }
  }
  return 0;
}

/// Get the content of a file, starting from the first non-whitespace character
/// after the import statement.
static char *get_content(char *file_path) {
  char *full_contents = read_file(file_path);
  char *contents = strchr(full_contents, 'i');
  if (is_import(full_contents, contents)) {
    return strchr(full_contents, '}') + 1;
  }
  return full_contents;
}

/// Get all imports recursively, read their contents, and add them to the
/// Imports structure.
static bool get_all_imports(Imports *imports) {
  bool added = true;
  for (size_t i = 0; i < imports->length; ++i) {
    if (imports->is_imported[i]) {
      continue;
    }
    added = false;
    char *file_contents = read_file(imports->imports[i]);
    const char *start = strchr(file_contents, 'i');

    if (start != NULL && is_import(file_contents, start)) {
      start += 6;
      start = skip_whitespace(start);

      switch (*start) {
      case '{': {
        char *c = malloc(strlen(start) + 1);
        strcpy(c, start);
        char *importss = grab_imports(c + 1);

        size_t length = 0;
        char delimiter[] = ",";
        char *comma = ",";
        char **import_list = split_string(importss, delimiter, &length);

        for (size_t i = 0; i < length; ++i) {
          char *current_import = get_abs_file_path(import_list[i]);
          write_import(imports, current_import, get_content(current_import));
        }
        free(c);
        imports->is_imported[i] = true;
        break;
      }
      default:
        printf("Import must have a body.\n");
        return NULL;
      }
    }
    imports->is_imported[i] = true;
  }
  return added;
}

/// Sort the contents of the Imports structure based on import count using
/// bubble sort.
static void sort_contents(Imports *imports) {
  for (size_t i = 0; i < imports->length - 1; ++i) {
    for (size_t j = i; j < imports->length - 1; ++j) {
      if (imports->import_count[j] > imports->import_count[j + 1]) {
        size_t cnt = imports->import_count[j];

        imports->import_count[j] = imports->import_count[j + 1];

        imports->import_count[j + 1] = cnt;
      }
    }
  }
}

/// Get the total length of contents in the Imports structure.
static size_t get_length(Imports *imports) {
  size_t length = 0;
  for (size_t i = 0; i < imports->length; ++i) {
    length += strlen(imports->contents[i]);
  }
  return length;
}

/// Merge the contents of the Imports structure into a single string.
static char *merge_imports(Imports *imports) {
  size_t length = get_length(imports);
  char *merged = calloc(length + 1, 1);
  for (ssize_t i = imports->length - 1; i >= 0; --i) {
    merged = strcat(merged, imports->contents[i]);
  }
  return merged;
}

// /// Combine files, identify import statements, process each import
// recursively, and merge their contents.
char *combine_files(char *file_path) {
  char *path = strdup(file_path);
  char *file_contents = read_file(file_path);
  const char *start = strchr(file_contents, 'i');

  if (start != NULL && is_import(file_contents, start)) {
    start += 6;
    start = skip_whitespace(start);

    switch (*start) {
    case '{': {
      char *c = malloc(strlen(start) + 1);
      strcpy(c, start);
      char *imports = grab_imports(c + 1);

      size_t length = 0;
      char delimiter[] = ",";
      char *comma = ",";
      char **import_list = split_string(imports, delimiter, &length);

      Imports im;
      init_imports(&im);
      write_import(&im, get_abs_file_path(remove_suffix(path, ".salmon")),
                   get_content(file_path));
      while (!get_all_imports(&im))
        ;
      sort_contents(&im);
      char *result = merge_imports(&im);
      free_imports(&im);
      return result;
    }
    default:
      printf("Import must have a body.\n");
      free(path);
      return NULL;
    }
  }
  free(path);
  return file_contents;
}
