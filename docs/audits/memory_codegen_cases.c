// Small inputs for inspecting unoptimized CodeGen, not performance benchmarks.
void touch(int *p);

int straightline(int x) {
  int y = x + 1;
  return y * 2;
}

int reassign(int x) {
  int y = x;
  y += 1;
  y *= 2;
  return y;
}

int readonly_branch(int condition, int x) {
  if (condition)
    return x;
  return x + 1;
}

int merge(int condition, int x) {
  int result = x;
  if (condition)
    result = x + 1;
  else
    result = x + 2;
  return result;
}

int pointer_local(int *p) {
  int *q = p;
  return *q;
}

int address_local(int x) {
  int y = x;
  return *(&y);
}

int alias_call(int x) {
  int y = x;
  touch(&y);
  return y;
}

int alias_store(int *p, int *q) {
  int old = *p;
  *q = 7;
  return *p + old;
}

int boolean_branch(int x) {
  if (x < 10)
    return 1;
  return 0;
}

int comparison_value(int x) { return x < 10; }

int repeated_index(int *p, int i) {
  p[i] = p[i] + 1;
  return p[i];
}

int repeated_array(int value) {
  int a[2];
  a[1] = value;
  return a[1] + a[1];
}

int loop_temp(int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    int temp = i + 1;
    sum += temp;
  }
  return sum;
}

int pointer_difference(int *first, int *last) { return last - first; }
