#include <stdio.h>

int main(int ac, char **av) {
	if (ac == 0)
		return 0;

	for (int i = 0; i < 3; i++)
		printf("i = %d", i);

	return 0;
}
