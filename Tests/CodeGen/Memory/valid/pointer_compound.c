int main(void) {
  int a[2];
  a[0] = 5;
  a[1] = 7;
  int i = 0;
  if ((a[i++] += 3) != 8)
    return 1;
  if (i != 1)
    return 2;
  int *p = a;
  if ((*p *= 4) != 32)
    return 3;
  if ((*p -= 2) != 30)
    return 4;
  if (a[0] != 30)
    return 5;
  if (a[1] != 7)
    return 6;
  unsigned char c = 250;
  unsigned char *byte = &c;
  if ((*byte += 10) != 4)
    return 7;
  if (c != 4)
    return 8;
  if ((*byte *= 100) != 144)
    return 9;
  if ((*byte -= 145) != 255)
    return 10;
  int n = 0;
  for (int j = 0; j < 3; j += 1)
    n += a[1];
  if (n != 21)
    return 11;
  return 0;
}
