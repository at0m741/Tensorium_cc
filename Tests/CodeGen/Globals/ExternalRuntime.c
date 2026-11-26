int external_value = 3;
extern int defined_here;
int update_external(void);
int main(void) {
  if (update_external() != 7) return 1;
  if (external_value != 7) return 2;
  return defined_here - 4;
}
