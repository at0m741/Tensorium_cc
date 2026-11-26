extern int external_value;
int defined_here = 4;
int update_external(void) {
  external_value += defined_here;
  return external_value;
}
