#include <stdio.h>

int count(int n);

int main(void) {
  const struct {
    int input;
    int expected;
  } cases[] = {{-3, 0}, {0, 0}, {1, 1}, {2, 2}, {3, 2},
               {4, 3},  {5, 4}, {6, 4}, {10, 4}};

  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    int actual = count(cases[i].input);
    if (actual != cases[i].expected) {
      fprintf(stderr, "count(%d): expected %d, got %d\n", cases[i].input,
              cases[i].expected, actual);
      return 1;
    }
  }
  return 0;
}
