static void redirect(int **slot, int *value) { *slot = value; }

static void parameter_address(int *p, int *q) {
  redirect(&p, q);
  *p = 31;
}

int main(void) {
  int x = 1;
  int y = 2;
  int *p = &x;
  redirect(&p, &y);
  *p = 27;
  if (y != 27)
    return 1;
  parameter_address(&x, &y);
  if (x != 1)
    return 2;
  if (y != 31)
    return 3;
  return 0;
}
