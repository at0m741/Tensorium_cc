static void choose(int *p, int condition) {
  if (condition)
    *p = 7;
  else
    *p = 9;
}

static void add_n(int *p, int n) {
  while (n > 0) {
    *p = *p + 1;
    n = n - 1;
  }
}

int main(void) {
  int x = 0;
  choose(&x, 0);
  if (x != 9)
    return 1;
  choose(&x, 1);
  if (x != 7)
    return 2;
  add_n(&x, 0);
  if (x != 7)
    return 3;
  add_n(&x, 12);
  if (x != 19)
    return 4;
  return 0;
}
