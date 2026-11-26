int zero;
int values[3];
int matrix[2][3];
int *pointer;
int *explicit_null = 0;
float unused_float;
int main(void) {
  if (zero) return 1;
  if (values[2]) return 2;
  if (matrix[1][2]) return 3;
  if (pointer) return 4;
  if (explicit_null) return 5;
  pointer = &zero;
  *pointer = 7;
  values[1] = 3;
  return zero + values[1] - 10;
}
