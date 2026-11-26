/* Integration of static array addresses, void pointers and native LP64 write. */
typedef long ssize_t;
typedef unsigned long size_t;
ssize_t write(int descriptor, const void *buffer, size_t count);

char buffer[5];
void *slice = buffer + 1;
const void *readonly_slice = &buffer[1];
void *one_past = buffer + 5;
int matrix[2][3];
void *element = &matrix[1][2];

int main(void) {
  char *bytes = slice;
  bytes[0] = 72;
  bytes[1] = 105;
  bytes[2] = 10;
  int *typed_element = element;
  *typed_element = 7;
  if (matrix[1][2] != 7) return 1;
  if (slice != &buffer[1]) return 2;
  if (one_past != &buffer[5]) return 3;
  if (write(1, readonly_slice, 3) != 3) return 4;
  if (write(1, one_past, 0) != 0) return 5;
  return 0;
}
