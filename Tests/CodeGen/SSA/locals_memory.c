int local_reassigned(int x) {
  int y = x;
  y += 2;
  return y;
}

int local_uninitialized(int x) {
  int y;
  y = x;
  return y;
}

int local_branch_write(int condition, int x) {
  int y = x;
  if (condition)
    y = 7;
  return y;
}

int local_address(int x) {
  int y = x;
  int *p = &y;
  *p = 9;
  return y;
}

int local_pointer_alias(int x) {
  int y = x;
  int *p = &y;
  int *q = p;
  *q += 2;
  return y;
}

int local_pointer_rebind(int *a, int *b) {
  int *p = a;
  p = b;
  *p = 11;
  return *a + *b;
}

int local_address_in_loop(int x, int count) {
  int value = x;
  for (int i = 0; i < count; ++i) {
    int *p = &value;
    *p += 1;
  }
  return value;
}

static int bump(int *p) {
  ++*p;
  return *p;
}

int local_initializer_address(int x) {
  int y = bump(&x);
  return y + y;
}

int local_copy_of_mutable(int x) {
  int y = x;
  ++x;
  return y + x;
}

int local_array(int x) {
  int values[2];
  values[0] = x;
  values[1] = x + 1;
  return values[0] + values[1];
}

int main(void) {
  if (local_reassigned(3) != 5)
    return 1;
  if (local_uninitialized(8) != 8)
    return 2;
  if (local_branch_write(0, 3) != 3)
    return 3;
  if (local_branch_write(1, 3) != 7)
    return 4;
  if (local_address(3) != 9)
    return 5;
  if (local_pointer_alias(5) != 7)
    return 6;

  int a = 2;
  int b = 3;
  if (local_pointer_rebind(&a, &b) != 13)
    return 7;
  if (a != 2)
    return 8;
  if (b != 11)
    return 9;
  if (local_address_in_loop(4, 0) != 4)
    return 10;
  if (local_address_in_loop(4, 3) != 7)
    return 11;
  if (local_initializer_address(3) != 8)
    return 12;
  if (local_copy_of_mutable(3) != 7)
    return 13;
  if (local_array(4) != 9)
    return 14;
  return 0;
}
