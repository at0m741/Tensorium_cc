// Function pointer and typedef playground
typedef double (*math_cb)(int, float);

math_cb compute;

double add_scaled(int a, float b) {
  return a + b;
}
