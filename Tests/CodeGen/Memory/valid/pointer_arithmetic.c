static int sum(int *p, int count) {
  int result = 0;
  for (int i = 0; i < count; ++i)
    result = result + p[i];
  return result;
}

int main(void) {
  int a[4];
  for (int i = 0; i < 4; ++i)
    a[i] = i + 1;
  if (sum(a, 4) != 10)
    return 1;
  int *p = a + 2;
  if (p[-1] != 2)
    return 2;
  if (*(1 + a) != 2)
    return 3;
  if (2 [a] != 3)
    return 4;
  if (p - a != 2)
    return 5;
  if (a - p != -2)
    return 6;
  p = a + 4;
  if (p - a != 4)
    return 7;
  if (p <= a)
    return 8;
  p -= 2;
  p += 1;
  if (*p != 4)
    return 9;
  if (sizeof a != 4 * sizeof(int))
    return 10;
  short s[3];
  s[0] = 12;
  s[1] = 23;
  s[2] = 34;
  short *q = s + 2;
  if (q - s != 2)
    return 11;
  if (q[-1] != 23)
    return 12;
  return 0;
}
