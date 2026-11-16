int main(void) {
  int x = 3;
  int y = 7;
  int *a[2];
  a[0] = &x;
  a[1] = &y;
  int **p = a;
  **p = 11;
  p = p + 1;
  **p = 19;
  if (x != 11)
    return 1;
  if (y != 19)
    return 2;
  if (p - a != 1)
    return 3;
  if (sizeof a != 2 * sizeof(int *))
    return 4;
  return 0;
}
