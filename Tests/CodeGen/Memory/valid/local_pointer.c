int main(void) {
  int x = 1;
  int y = 7;
  int *p = &x;
  *p = 42;
  if (x != 42)
    return 1;
  p = &y;
  *p = 9;
  if (x + y != 51)
    return 2;
  int *q = p;
  *q = *q + 1;
  if (y != 10)
    return 3;
  return 0;
}
