#include <stdint.h>
#include <string.h>

extern int *cc1_identity(int *p);
extern void cc1_set(int **slot, int *value);

void external_set(int *p, int value) { *p = value; }

int *external_identity(int *p) {
  int *result = 0;
  cc1_set(&result, cc1_identity(p));
  return result;
}

int external_read(const int *p) { return *p; }

void external_init_float(float *p) {
  // A NaN payload makes accidental arithmetic/conversion observable.
  uint32_t bits = UINT32_C(0x7fc12345);
  memcpy(&p[1], &bits, sizeof bits);
}

void external_init_double(double *p) {
  uint64_t bits = UINT64_C(0x8000000000000000);
  memcpy(&p[1], &bits, sizeof bits);
}

int external_check_float(const float *p) {
  uint32_t bits;
  memcpy(&bits, p, sizeof bits);
  return bits == UINT32_C(0x7fc12345);
}

int external_check_double(const double *p) {
  uint64_t bits;
  memcpy(&bits, p, sizeof bits);
  return bits == UINT64_C(0x8000000000000000);
}
