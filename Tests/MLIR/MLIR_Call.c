#include <stdio.h>

int add(int a, int b) {
	return a + b;
}

int sub(int a, int b) {
	return a - b;
}

int mul(int a, int b) {
	return a * b;
}

int main() {
	int a = 1;
	int b = 4;

	add(a, b);
	printf("hello");
	return 0;
}
