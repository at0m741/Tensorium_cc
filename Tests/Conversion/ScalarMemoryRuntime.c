#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern int tensorium_memory_i32(int value, int condition);
extern double tensorium_memory_f64(double value);

int main(void) {
  const int integers[] = {0, 1, -1, 42, INT_MIN, INT_MAX};
  for (size_t i = 0; i < sizeof(integers) / sizeof(integers[0]); ++i) {
    for (int branch = 0; branch < 2; ++branch) {
      if (tensorium_memory_i32(integers[i], branch) != integers[i]) {
        fprintf(stderr, "integer storage/call/branch roundtrip failed\n");
        return 1;
      }
    }
  }
  const double floats[] = {0.0, -0.0, 1.5, -31.25};
  for (size_t i = 0; i < sizeof(floats) / sizeof(floats[0]); ++i) {
    double result = tensorium_memory_f64(floats[i]);
    if (memcmp(&result, &floats[i], sizeof(result)) != 0) {
      fprintf(stderr, "floating-point storage roundtrip failed\n");
      return 1;
    }
  }
  return 0;
}
