int data[4];

unsigned long sizes(void) {
  return sizeof data + sizeof(data[1]);
}
