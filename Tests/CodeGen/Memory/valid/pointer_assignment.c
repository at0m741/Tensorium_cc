static int reassign(int *p, int *q) {
  p = q;
  *p = 19;
  return *q;
}

int main(void) {
  int x = 1;
  int y = 2;
  if (reassign(&x, &y) != 19)
    return 1;
  if (x != 1)
    return 2;
  if (y != 19)
    return 3;
  return 0;
}
