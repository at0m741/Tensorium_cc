extern int external_values[4];
extern short external_shorts[4];
int *exported_mid = external_values + 2;
int *exported_end = &external_values[4];
short *exported_short = &external_shorts[3];
int update_external_array(void) {
  *exported_mid += 7;
  *exported_short += 3;
  return external_values[2];
}
