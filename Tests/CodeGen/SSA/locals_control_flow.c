int local_branch(int condition, int x) {
  int base = x + 1;
  if (condition) {
    int value = base * 2;
    return value;
  } else {
    int value = base + 3;
    return value;
  }
}

int local_shadow(int x) {
  int value = x + 1;
  {
    int value = x * 2;
    if (value > 0)
      return value;
  }
  return value;
}

int local_for(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int temp = i + 1;
    sum += temp;
  }
  return sum;
}

int local_while(int count) {
  int sum = 0;
  while (count > 0) {
    int temp = count * 2;
    sum += temp;
    --count;
  }
  return sum;
}

int local_for_seed(int count, int start) {
  for (int seed = start; count > 0; --count) {
    int value = seed + count;
    if (count == 1)
      return value;
  }
  return start;
}

int local_loop_jumps(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = i + 1;
    if (i == 1)
      continue;
    if (i == 4)
      break;
    sum += value;
  }
  return sum;
}

int main(void) {
  if (local_branch(1, 4) != 10)
    return 1;
  if (local_branch(0, 4) != 8)
    return 2;
  if (local_shadow(3) != 6)
    return 3;
  if (local_shadow(-3) != -2)
    return 4;
  if (local_for(0) != 0)
    return 5;
  if (local_for(1) != 1)
    return 6;
  if (local_for(4) != 10)
    return 7;
  if (local_for(1000) != 500500)
    return 8;
  if (local_while(0) != 0)
    return 9;
  if (local_while(4) != 20)
    return 10;
  if (local_for_seed(0, 7) != 7)
    return 11;
  if (local_for_seed(3, 7) != 8)
    return 12;
  if (local_loop_jumps(0) != 0)
    return 13;
  if (local_loop_jumps(3) != 4)
    return 14;
  if (local_loop_jumps(7) != 8)
    return 15;
  return 0;
}
