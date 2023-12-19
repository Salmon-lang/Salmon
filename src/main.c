#include "import.h"
#include <stdlib.h>
int main(void) {
  free(combine_files("test.salmon"));
  return 0;
}
