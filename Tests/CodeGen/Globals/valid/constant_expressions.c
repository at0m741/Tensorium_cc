int answer = (2 + 5) * 6;
short narrow = -7;
unsigned int wrapped = 4294967295U + 1U;
long long large = 9223372036854775807LL;
const int readonly = 17;
int main(void) {
  if (answer != 42) return 1;
  if (narrow != -7) return 2;
  if (wrapped != 0U) return 3;
  if (large != 9223372036854775807LL) return 4;
  return readonly - 17;
}
