int main(void) {
  int a[3];
  a[0] = 10;
  a[1] = 20;
  a[2] = 30;
  int *p = a;
  if (*p++ != 10)
    return 1;
  if (*++p != 30)
    return 2;
  if (*p-- != 30)
    return 3;
  if (*--p != 10)
    return 4;
  int **pp = &p;
  (*pp)++;
  if (*p != 20)
    return 5;
  if (++*p != 21)
    return 6;
  if ((*p)-- != 21)
    return 7;
  if (*p != 20)
    return 8;
  int i = 0;
  ++a[i++];
  if (i != 1)
    return 9;
  if (a[0] != 11)
    return 10;
  if (sizeof *p++ != sizeof(int))
    return 11;
  if (p != a + 1)
    return 12;
  int *slots[1];
  slots[0] = a;
  int j = 0;
  slots[j++] += 1;
  if (j != 1)
    return 13;
  if (slots[0] != a + 1)
    return 14;
  return 0;
}
