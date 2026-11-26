long generated_word(long value) { return value; }
long long generated_wide(long long value) { return value; }
unsigned long long generated_unsigned(unsigned long long value) { return value; }
void *generated_identity(void *pointer) { return pointer; }
int generated_recover(const void *pointer) {
  const int *typed = pointer;
  return *typed;
}
long long native_wide(void *pointer, long long value);
long long call_native(void *pointer) {
  return native_wide(pointer, 4294967296LL);
}
