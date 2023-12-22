#include "import.h"
#include <stdio.h>
#include <stdlib.h>
int main(void) {
  char *src_code = combine_files("test.salmon");
  free(src_code);
  return 0;
}
