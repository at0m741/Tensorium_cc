static void set_short(short *p, int value) {
  *p = value;
}

static int read_short(const short *p) {
  return *p;
}

int main(void) {
  short x = 0;
  set_short(&x, 42);
  if (read_short(&x) != 42)
    return 1;
  set_short(&x, 65535);
  // Check truncation on store and sign extension on load without unary minus.
  if (read_short(&x) + 1 != 0)
    return 2;
  return 0;
}
