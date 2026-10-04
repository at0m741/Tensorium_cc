#define HEADER "include/values.h"
#include HEADER
#include "include/values.h"
#include "include/once.h"
#include "include/./once.h"

#if defined(BASE) && MORE && !defined(MISSING)
int main(void) {
  return BASE + MORE;
}
#else
#error wrong branch
#endif
