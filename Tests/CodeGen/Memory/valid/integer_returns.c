long signed_word(long value) { return value; }
unsigned long unsigned_word(unsigned long value) { return value; }
long long signed_wide(long long value) { return value; }
unsigned long long unsigned_wide(unsigned long long value) { return value; }
unsigned int unsigned_int(unsigned int value) { return value; }
int main(void) {
  if (signed_word(-1) != -1) return 1;
  if (signed_wide(4294967296LL) != 4294967296LL) return 2;
  if (unsigned_word(7U) != 7U) return 3;
  if (unsigned_wide(4294967296ULL) != 4294967296ULL) return 4;
  if (unsigned_int(4294967295U) != 4294967295U) return 5;
  return 0;
}
