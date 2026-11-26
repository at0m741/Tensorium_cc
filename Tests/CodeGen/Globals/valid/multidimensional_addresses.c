int matrix[2][3];
int *element = &matrix[1][2];
int *row_begin = matrix[1];
int *row_end = &matrix[1][3];
int *row_back = &matrix[1][3] - 1;
int (*row)[3] = matrix + 1;
int (*whole)[2][3] = &matrix;
int (*whole_end)[2][3] = &matrix + 1;
int (*whole_back)[2][3] = (&matrix + 1) - 1;
int main(void) {
  *element = 7;
  if (matrix[1][2] != 7) return 1;
  if (row_end != row_begin + 3) return 2;
  if (row_back != element) return 3;
  row[0][1] = 9;
  if (matrix[1][1] != 9) return 4;
  if (whole != &matrix) return 5;
  if (whole_end != whole + 1) return 6;
  if (whole_back != whole) return 7;
  return 0;
}
