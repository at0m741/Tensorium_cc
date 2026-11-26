extern int counter;
int counter;
int counter = 3;
extern int counter;
int counter;
static int hidden;
extern int hidden;
static int hidden;
int read_counter(void) { return counter; }
int main(void) {
  int counter = 9;
  hidden = 4;
  return counter + read_counter() + hidden - 16;
}
