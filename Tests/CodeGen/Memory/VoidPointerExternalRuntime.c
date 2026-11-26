long generated_word(long value);
long long generated_wide(long long value);
unsigned long long generated_unsigned(unsigned long long value);
void *generated_identity(void *pointer);
int generated_recover(const void *pointer);
long long call_native(void *pointer);

long long native_wide(void *pointer, long long value) {
  int *typed = pointer;
  *typed = 5;
  return value + 1;
}
int main(void) {
  int value = 3;
  if (generated_word(-7) != -7) return 1;
  if (generated_wide(-4294967297LL) != -4294967297LL) return 2;
  if (generated_unsigned(18446744073709551615ULL) != 18446744073709551615ULL) return 3;
  if (generated_identity(&value) != &value) return 4;
  if (generated_recover(&value) != 3) return 5;
  if (call_native(&value) != 4294967297LL) return 6;
  return value - 5;
}
