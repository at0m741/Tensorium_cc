extern int provided;
int *exported_pointer = &provided;
static int hidden = 4;
int *exported_hidden = &hidden;

int update_provided(void) {
  *exported_pointer += *exported_hidden;
  return provided;
}
