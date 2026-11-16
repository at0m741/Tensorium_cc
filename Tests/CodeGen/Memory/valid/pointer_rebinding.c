static void choose(int *p, int *q, int condition) {
  if (condition)
    p = q;
  *p = 17;
}

static void alternate(int **p, int *first, int *second, int count) {
  while (count > 0) {
    if (count == 2)
      *p = second;
    else
      *p = first;
    **p = **p + 1;
    count = count - 1;
  }
}

int main(void) {
  int x = 0;
  int y = 0;
  choose(&x, &y, 0);
  if (x != 17)
    return 1;
  choose(&x, &y, 1);
  if (y != 17)
    return 2;
  int *p = &x;
  alternate(&p, &x, &y, 3);
  if (x != 19)
    return 3;
  if (y != 18)
    return 4;
  if (p != &x)
    return 5;
  return 0;
}
