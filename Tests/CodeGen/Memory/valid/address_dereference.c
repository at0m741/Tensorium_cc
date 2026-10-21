static void set(int *p, int value) {
  *p = value;
}

int main(void) {
  int x = 1;
  set(&x, 42);
  if (x != 42)
    return 1;
  if (*(&x) != 42)
    return 2;
  set(&*(&x), 11);
  if (x != 11)
    return 3;
  return 0;
}
