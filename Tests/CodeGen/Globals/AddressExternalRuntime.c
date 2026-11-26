int provided = 3;
extern int *exported_pointer;
extern int *exported_hidden;
int update_provided(void);

int main(void) {
  if (exported_pointer != &provided) return 1;
  if (*exported_hidden != 4) return 2;
  if (update_provided() != 7) return 3;
  return provided - 7;
}
