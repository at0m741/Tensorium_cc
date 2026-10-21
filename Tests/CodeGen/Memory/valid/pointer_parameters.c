static void set(int *p, int value) {
  *p = value;
}

static int read(const int *p) {
  return *p;
}

static int forward(int *p) {
  return read(p);
}

static int parameter_address(int x) {
  set(&x, 31);
  return x;
}

int main(void) {
  int x = 6;
  if (read(&x) != 6)
    return 1;
  if (forward(&x) != 6)
    return 2;
  if (parameter_address(0) != 31)
    return 3;
  return 0;
}
