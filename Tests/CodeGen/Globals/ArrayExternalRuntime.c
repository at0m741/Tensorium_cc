int external_values[4] = {1, 2, 3, 4};
short external_shorts[4] = {5, 6, 7, 8};
extern int *exported_mid;
extern int *exported_end;
extern short *exported_short;
int update_external_array(void);
int main(void) {
  if (exported_mid != &external_values[2]) return 1;
  if (exported_end != &external_values[4]) return 2;
  if (exported_short != &external_shorts[3]) return 3;
  if (update_external_array() != 10) return 4;
  return external_shorts[3] - 11;
}
