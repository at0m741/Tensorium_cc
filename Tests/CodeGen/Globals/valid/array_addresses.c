int values[4];
int *begin = values;
int *third = &values[2];
int *end = values + 4;
int *back = (values + 4) - 2;
int *commuted = 2 + values;
int *negative = values - (-2);
int *index_commuted = &2[values];
int *wrapped = values + (9223372036854775807ULL + 9223372036854775807ULL + 2ULL);
short shorts[4];
short *short_pointer = &shorts[2];
int *links[3];
int **link_pointer = &links[2];
const int readonly[2];
const int *readonly_pointer = readonly + 1;
extern int later[];
int *later_pointer = &later[3];
int later[4];

int main(void) {
  if (begin != &values[0]) return 1;
  if (third != begin + 2) return 2;
  if (end != &values[4]) return 3;
  if (back != third) return 4;
  if (commuted != third) return 5;
  if (negative != third) return 6;
  if (index_commuted != third) return 7;
  if (wrapped != begin) return 13;
  *third = 12;
  if (values[2] != 12) return 8;
  *short_pointer = 7;
  if (shorts[2] != 7) return 9;
  *link_pointer = &values[1];
  if (links[2] != begin + 1) return 10;
  if (readonly_pointer != &readonly[1]) return 11;
  *later_pointer = 8;
  if (later[3] != 8) return 12;
  return 0;
}
