static void update_pair(int *first, int *second) {
  *first = 1;
  *second = *first + 41;
}

int main(void) {
  int x = 0;
  int y = 0;
  update_pair(&x, &y);
  if (x != 1)
    return 1;
  if (y != 42)
    return 2;
  update_pair(&x, &x);
  if (x != 42)
    return 3;
  return 0;
}
