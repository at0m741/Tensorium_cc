#define STR(x) #x
#define HEADER(path) STR(path)
#include HEADER(include/macros.h)
#define CAT(a,b) a ## b
#define SUM(x,y) ((x) + (y))
#define RETURN(...) return __VA_ARGS__
#define EMPTY()

#if SUM(MACRO_BASE,2) == 8 && defined(__LINE__)
int CAT(mac,ro)(int value) {
  RETURN(SUM(value, MACRO_BASE));
}
const char *description(void) {
  return "macro: " STR(SUM(value, MACRO_BASE));
}
int line(void) {
  EMPTY()
  return __LINE__;
}
#else
#error function-like macros did not expand in #if
#endif
