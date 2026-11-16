static void set(int (*row)[3], int value) { row[1][2] = value; }

int main(void) {
  int a[2][3];
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      a[i][j] = i * 10 + j;
  if (a[1][2] != 12)
    return 1;
  int (*p)[3] = a;
  p = p + 1;
  if ((*p)[1] != 11)
    return 2;
  if (p - a != 1)
    return 3;
  set(a, 42);
  if (a[1][2] != 42)
    return 4;
  if (sizeof a != 6 * sizeof(int))
    return 5;
  if (sizeof *p != 3 * sizeof(int))
    return 6;
  return 0;
}
