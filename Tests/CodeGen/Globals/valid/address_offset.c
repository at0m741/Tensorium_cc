int object;
int *pointer = &object + 1;
int *back = (&object + 1) - 1;
int main(void) {
  if (pointer != &object + 1) return 1;
  if (back != &object) return 2;
  *back = 3;
  return object - 3;
}
