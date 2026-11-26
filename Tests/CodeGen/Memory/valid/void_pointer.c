int global = 9;
void *global_address = &global;
void *global_null = 0;

void *identity(void *pointer) { return pointer; }
int *recover(void *pointer) {
  int *typed = pointer;
  return typed;
}
int read_if_present(const void *pointer) {
  if (!pointer) return 0;
  const int *typed = pointer;
  return *typed;
}
int main(void) {
  int local = 3;
  void *pointer = &local;
  int *typed = identity(pointer);
  *typed = 5;
  if (local != 5) return 1;
  void **slot = &pointer;
  *slot = &global;
  typed = *slot;
  *typed = 11;
  if (global != 11) return 2;
  if (global_address != pointer) return 3;
  if (global_null) return 4;
  if (recover(pointer) != &global) return 5;
  if (read_if_present(pointer) != 11) return 6;
  if (read_if_present(0) != 0) return 7;
  return 0;
}
