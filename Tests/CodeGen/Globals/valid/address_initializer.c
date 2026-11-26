int object;
int *pointer = &object;

int main(void) {
  *pointer = 5;
  return object - 5;
}
