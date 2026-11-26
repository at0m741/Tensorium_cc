int counter = 3;
int *pointer = &counter;
int **indirect = &pointer;
int *const fixed = &counter;
const int *readonly_view = &counter;

static int hidden = 4;
int *visible = &hidden;

extern int later;
int *forward = &later;
int later = 9;

int main(void) {
  if (pointer != &counter) return 1;
  if (indirect != &pointer) return 2;
  **indirect += 2;
  if (counter != 5) return 3;
  if (*readonly_view != 5) return 4;
  *fixed += 1;
  if (counter != 6) return 5;
  *visible += 3;
  if (hidden != 7) return 6;
  if (*forward != 9) return 7;
  pointer = &later;
  **indirect += 1;
  if (later != 10) return 8;
  return 0;
}
