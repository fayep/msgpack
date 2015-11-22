#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"

void describe(char *name, char *data) {
  test_name = name;
  test_data = (unsigned char *)data;
}

int int_to_eql_expect(int a, int b) {
  sprintf(test_result, "%d == %d", a, b);
  return a == b;
}

int string_to_eql_expect(char *a, char *b, int length) {
  sprintf(test_result, "'%s' == '%s'", a, b);
  return strncmp(a,b,length)==0;
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

/* we define a weak testsuite here to resolve the testsuite requirement */
__attribute__((weak)) void testsuite(void) {

  describe("result", 0);
  sprintf(test_result, "minimal test");
  result(1==1);
  describe("int_to_eql_expect", "");
  expect(int_to_eql, 0, *test_data);
  describe("string_to_eql_expect", "result");
  expect(string_to_eql, "result", (char *)test_data, strlen((char *)test_data) + 1);

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
