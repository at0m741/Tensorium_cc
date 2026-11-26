#include "api.h"
int main(void) {
  if (add(shared, 2) != 5) return 1;
  shared += 4;
  return shared - 7;
}
