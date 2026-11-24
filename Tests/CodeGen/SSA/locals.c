int local_scalar(int x) {
  int y = x + 1;
  return y * 2;
}

int local_chain(int x) {
  int a = x + 1;
  int b = a * 2;
  int c = b - 3;
  return c;
}

int local_const(int x) {
  const int y = x + 1;
  return y;
}

int local_narrow(short x) {
  short y = x;
  return y;
}

int local_pointer(int *p) {
  int *q = p;
  return *q;
}

int local_snapshot(int *p) {
  int before = *p;
  *p = 9;
  return before + *p;
}

static int next(int *p) {
  *p += 1;
  return *p;
}

int local_call(int *p) {
  int y = next(p);
  return y + y;
}

int local_initializer_write(int x) {
  int y = (x = 3);
  return y + x;
}

int local_sizeof_self(void) {
  int y = sizeof(y);
  return y;
}

int local_sizeof_effects(int x) {
  int y = x;
  sizeof(y++);
  sizeof(&y);
  return y;
}

int main(void) {
  if (local_scalar(4) != 10)
    return 1;
  if (local_scalar(-3) != -4)
    return 2;
  if (local_chain(5) != 9)
    return 3;
  if (local_const(7) != 8)
    return 4;
  if (local_narrow(-7) != -7)
    return 5;

  int value = 4;
  if (local_pointer(&value) != 4)
    return 6;
  if (local_snapshot(&value) != 13)
    return 7;
  if (value != 9)
    return 8;
  value = 4;
  if (local_call(&value) != 10)
    return 9;
  if (value != 5)
    return 10;
  if (local_initializer_write(0) != 6)
    return 11;
  if (local_sizeof_self() != sizeof(int))
    return 12;
  if (local_sizeof_effects(7) != 7)
    return 13;
  return 0;
}
