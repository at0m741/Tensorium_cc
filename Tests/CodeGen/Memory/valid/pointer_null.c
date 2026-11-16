static int is_null(int *p) {
  if (p)
    return 0;
  return 1;
}

int main(void) {
  int *p = 0;
  if (p != 0)
    return 1;
  if (!is_null(p))
    return 2;
  int x = 42;
  p = &x;
  if (is_null(p))
    return 3;
  if (!p)
    return 4;
  int **pp = 0;
  if (pp)
    return 5;
  pp = &p;
  if (*pp != &x)
    return 6;
  return 0;
}
