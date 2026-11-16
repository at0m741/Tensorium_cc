void external_set(int *p, int value);
int *external_identity(int *p);
int external_read(const int *p);
void external_init_float(float *p);
void external_init_double(double *p);
int external_check_float(const float *p);
int external_check_double(const double *p);

int *cc1_identity(int *p) { return p; }

void cc1_set(int **slot, int *value) { *slot = value; }

int main(void) {
  int x = 1;
  external_set(&x, 42);
  if (external_read(&x) != 42)
    return 1;
  int *p = external_identity(&x);
  if (p != &x)
    return 2;
  *p = 19;
  if (x != 19)
    return 3;
  float floats[2];
  float float_result;
  external_init_float(floats);
  float_result = floats[1];
  if (external_check_float(&float_result) != 1)
    return 4;
  double doubles[2];
  double double_result;
  external_init_double(doubles);
  double_result = doubles[1];
  if (external_check_double(&double_result) != 1)
    return 5;
  return 0;
}
