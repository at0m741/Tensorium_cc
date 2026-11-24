int consume(int value);
int consume_address(int *value);

int choose(int condition, int x) {
  if (condition)
    return x;
  return x + 1;
}

int reassigned(int condition, int x) {
  if (condition)
    x = 42;
  return x;
}

int compound_assignments(int add, int sub, int mul, int div, int mod,
                         int bitand, int bitor, int bitxor, int left, int right,
                         int value) {
  add += value;
  sub -= value;
  mul *= value;
  div /= value;
  mod %= value;
  bitand &= value;
  bitor |= value;
  bitxor ^= value;
  left <<= value;
  right >>= value;
  return add + sub + mul + div + mod + bitand + bitor + bitxor + left + right;
}

int updates(int pre_inc, int pre_dec, int post_inc, int post_dec) {
  ++pre_inc;
  --pre_dec;
  post_inc++;
  post_dec--;
  return pre_inc + pre_dec + post_inc + post_dec;
}

int address_scalar(int x) {
  int *p = &x;
  return *p;
}

int pointer_write(int *p, int value) {
  *p = value;
  ++*p;
  return *p;
}

int pointer_rebind(int *p, int *q) {
  p = q;
  return *p;
}

int pointer_update(int *p, int value) {
  *p++ = value;
  return *p;
}

int pointer_address(int *p) {
  int **slot = &p;
  return **slot;
}

int pointer_element(int *p, int index) {
  p[index] += 1;
  --p[index];
  return p[index];
}

int pointee_address(int *p, int index) {
  int *q = &p[index];
  return *q;
}

int index_update(int *p, int index) {
  p[index++] = 42;
  return index;
}

int sizeof_only(int x, int *p) {
  return sizeof(x++) + sizeof(*p++) + sizeof(&x);
}

int initializer_write(int x) {
  int local = (x = 1);
  return local;
}

int initializer_address(int x) {
  int local = consume_address(&x);
  return local + x;
}

int control_flow(int count, int x) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    if (i == x)
      continue;
    result += x;
    if (result > 100)
      break;
  }
  while (result < count)
    ++result;
  return result;
}

int for_initializer_write(int count) {
  for (count = 0; count < 3; ++count) {
  }
  return count;
}

int for_condition_write(int count) {
  for (; count--; ) {
    break;
  }
  return count;
}

int for_increment_write(int count) {
  for (int i = 0; i < 3; ++count)
    ++i;
  return count;
}

int while_write(int x) {
  while (x--) {
    break;
  }
  return x;
}

int call_update(int x) {
  return consume(x++);
}

int call_pointer(int (*callee)(int), int x) {
  return callee(x++);
}

int conditional_write(int condition, int x, int y) {
  return condition ? (x = 1) : y++;
}

int logical_write(int x, int y) {
  return x && ++y;
}

int comma_write(int x, int y) {
  return (x++, y);
}

int shadowed(int x) {
  {
    int x = 1;
    ++x;
    consume_address(&x);
  }
  return x;
}

int volatile_scalar(volatile int x) {
  return x;
}

int volatile_pointer(int *volatile p) {
  return *p;
}

int volatile_pointee(volatile int *p) {
  return *p;
}

int adjusted_array(int p[4]) {
  p[0] = 42;
  return p[0];
}

int const_scalar(const int x) {
  return x;
}

double arithmetic(short x, float y) {
  return x + y;
}
