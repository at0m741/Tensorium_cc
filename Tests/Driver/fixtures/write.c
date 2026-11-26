/* Native LP64 POSIX declaration; SDK headers are not yet imported by CC1. */
typedef long ssize_t;
typedef unsigned long size_t;
ssize_t write(int descriptor, const void *buffer, size_t count);

int main(void) {
  char buffer[3];
  buffer[0] = 72;
  buffer[1] = 105;
  buffer[2] = 10;
  ssize_t count = write(1, buffer, 3);
  if (count != 3) return 1;
  ssize_t failure = write(-1, buffer, 3);
  if (failure >= 0) return 2;
  if (failure != -1) return 4;
  if (write(1, 0, 0) != 0) return 3;
  return 0;
}
