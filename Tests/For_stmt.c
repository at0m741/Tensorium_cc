int count(int n) {
  int result = 0;

  for (int i = 0; i < n; i = i + 1) {
    if (i == 2)
      continue;
    if (i == 5)
      break;
    result = result + 1;
  }

  return result;
}
