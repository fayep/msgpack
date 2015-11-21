#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TEST
#include "test.h"

/* we define a weak testsuite here to resolve the testsuite requirement */
__attribute__((weak)) void testsuite(void) {}

void describe(char *name, char *data) {
  test_name = name;
  test_data = (unsigned char *)data;
}

int int_to_eql_expect(int a, int b) {
  sprintf(test_result, "%d == %d", a, b);
  return a == b;
}

void result(int success) {
  if (!success) {
    printf("%s(%s) failed: %s\n", test_name, test_data, test_result);
    exit(1);
  } else if (test_dots) {
    printf(".");
  } else if (test_verbose) {
    printf("%s(%s) success: %s.\n", test_name, test_data, test_result);
  }
}

__attribute__((weak)) int main(int argc, char **argv) {

  if (argc>1) {
    if (strcmp(argv[1],"-v")==0) test_verbose = 1;
    if (strcmp(argv[1],"-d")==0) test_dots = 1;
  }

  testsuite();
  if (test_verbose) {
    printf("All OK");
  }
  printf("\n");

  return 0;
}
