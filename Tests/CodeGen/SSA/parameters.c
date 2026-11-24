static int choose(int condition, int x) {
  if (condition)
    return x;
  return x + 1;
}

static int conditional_write(int condition, int x) {
  if (condition)
    x = 42;
  return x;
}

static void set(int *p, int value) {
  *p = value;
}

static int branch_address(int condition, int x) {
  if (condition)
    set(&x, 42);
  return x;
}

static int loop_address(int count, int x) {
  while (count > 0) {
    if (count == 2)
      set(&x, 42);
    --count;
  }
  return x;
}

static int while_readonly(int *p, int limit) {
  while (*p < limit)
    ++*p;
  return limit;
}

static int for_readonly(int *p, int limit) {
  for (; *p < limit; ++*p) {
  }
  return limit;
}

static int pointer_rebind(int condition, int *p, int *q) {
  if (condition)
    p = q;
  *p = 17;
  return *p;
}

static int update_pointer(int *p, int count) {
  while (count > 0) {
    ++p;
    --count;
  }
  return *p;
}

static int sizeof_readonly(int x, int *p) {
  sizeof(x++);
  sizeof(p++);
  sizeof(&x);
  if (x)
    return x + *p;
  return 0;
}

int main(void) {
  if (choose(1, 41) != 41)
    return 1;
  if (choose(0, 41) != 42)
    return 2;
  if (conditional_write(0, 7) != 7)
    return 3;
  if (conditional_write(1, 7) != 42)
    return 4;
  if (branch_address(0, 7) != 7)
    return 5;
  if (branch_address(1, 7) != 42)
    return 6;
  if (loop_address(0, 7) != 7)
    return 7;
  if (loop_address(3, 7) != 42)
    return 8;

  int x = 1;
  if (while_readonly(&x, 4) != 4)
    return 9;
  if (x != 4)
    return 10;
  if (while_readonly(&x, 2) != 2)
    return 11;
  if (x != 4)
    return 12;
  x = 0;
  if (for_readonly(&x, 3) != 3)
    return 13;
  if (x != 3)
    return 14;
  if (for_readonly(&x, 2) != 2)
    return 15;
  if (x != 3)
    return 16;

  x = 1;
  int y = 2;
  if (pointer_rebind(0, &x, &y) != 17)
    return 17;
  if (x != 17)
    return 18;
  if (y != 2)
    return 19;
  x = 1;
  if (pointer_rebind(1, &x, &y) != 17)
    return 20;
  if (x != 1)
    return 21;
  if (y != 17)
    return 22;

  int data[3];
  data[0] = 1;
  data[1] = 2;
  data[2] = 3;
  if (update_pointer(data, 0) != 1)
    return 23;
  if (update_pointer(data, 2) != 3)
    return 24;
  if (sizeof_readonly(3, data) != 4)
    return 25;
  if (sizeof_readonly(0, data) != 0)
    return 26;
  return 0;
}
