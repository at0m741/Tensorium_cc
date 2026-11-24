// Analyze storage decisions only. Do not generate or execute these functions:
// self-initialization reads indeterminate values, and some storage kinds are
// not supported by CodeGen yet.
int local_self_reference(void) {
  int value = value;
  return 0;
}

int local_nested_self_reference(void) {
  int value = value + 1;
  return 0;
}

int local_pointer_self_reference(void) {
  int *value = value;
  return 0;
}

int local_volatile(int x) {
  volatile int value = x;
  return value;
}

int local_static(void) {
  static int value = 1;
  return value;
}

int local_extern(void) {
  extern int value;
  return value;
}
