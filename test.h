#ifndef __TEST_H
#define __TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void testsuite(void);

int test_verbose, test_dots;
char *test_name;
unsigned char *test_data;
char test_result[1000];

void describe(char *name, char *data);
int int_to_eql_expect(int a, int b);
int string_to_eql_expect(char *a, char *b, int length);
void result(int success);

#define expect(type, args...) result( type ## _expect( args ));

#endif // ndefined __TEST_H
