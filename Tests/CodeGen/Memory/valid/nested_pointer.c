static int *identity(int *p) { return p; }

static void indirect(int ***p, int *value) {
  **p = identity(value);
  ***p = 42;
}

int main(void) {
  int x = 1;
  int y = 2;
  int *p = &x;
  int **pp = &p;
  indirect(&pp, &y);
  if (p != &y)
    return 1;
  if (x != 1)
    return 2;
  if (y != 42)
    return 3;
  return 0;
}
