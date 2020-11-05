#include <stdio.h>
#include "some_program_friend.c"

typedef struct another_some_struct {
	int jar;
	int flies;
} another_some_struct;

typedef struct some_struct {
	int a;
	int b;
	another_some_struct moses;
} some_struct;

void foo(int what) {
	int bar;
	some_struct strct;
    printf("%d\n", bar);
}

int main() {
	int i = 5;
	printf("hello world! %d\n", i);
	foo(i);
		printf("hello world! %d\n", i);
			printf("hello world! %d\n", i);
				printf("hello world! %d\n", i);
	cow();
}

