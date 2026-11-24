int loop_for_storage(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = i * 2;
    ++value;
    sum += value;
  }
  return sum;
}

int loop_while_storage(int count) {
  int sum = 0;
  while (count > 0) {
    int value = count * 2;
    ++value;
    sum += value;
    --count;
  }
  return sum;
}

int loop_address_storage(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = i * 2;
    int *p = &value;
    *p += 1;
    sum += value;
  }
  return sum;
}

int loop_array_storage(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int values[2];
    values[0] = i;
    values[1] = values[0] + 3;
    sum += values[0] + values[1];
  }
  return sum;
}

int loop_nested_storage(int count) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = i;
    ++value;
    for (int j = 0; j < 2; ++j) {
      int value = i + j;
      value += 2;
      sum += value;
    }
    sum += value;
  }
  return sum;
}

static int loop_next(int *counter, int base) {
  ++*counter;
  return base + *counter;
}

int loop_call_initializer(int count, int *counter) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = loop_next(counter, i * 2);
    ++value;
    sum += value;
  }
  return sum;
}

int loop_conditional_initializer(int count, int *counter) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    if (i == 1)
      continue;
    if (i == 4)
      break;
    int value = loop_next(counter, i * 2);
    ++value;
    sum += value;
  }
  return sum;
}

int loop_load_initializer(int count, int *p) {
  int sum = 0;
  for (int i = 0; i < count; ++i) {
    int value = *p;
    ++value;
    sum += value;
  }
  return sum;
}

int main(void) {
  if (loop_for_storage(0) != 0)
    return 1;
  if (loop_for_storage(4) != 16)
    return 2;
  if (loop_for_storage(1000) != 1000000)
    return 3;
  if (loop_while_storage(0) != 0)
    return 4;
  if (loop_while_storage(4) != 24)
    return 5;
  if (loop_address_storage(0) != 0)
    return 6;
  if (loop_address_storage(4) != 16)
    return 7;
  if (loop_array_storage(0) != 0)
    return 8;
  if (loop_array_storage(4) != 24)
    return 9;
  if (loop_nested_storage(0) != 0)
    return 10;
  if (loop_nested_storage(3) != 27)
    return 11;

  int counter = 0;
  if (loop_call_initializer(0, &counter) != 0)
    return 12;
  if (counter != 0)
    return 13;
  if (loop_call_initializer(3, &counter) != 15)
    return 14;
  if (counter != 3)
    return 15;

  counter = 0;
  if (loop_conditional_initializer(0, &counter) != 0)
    return 16;
  if (counter != 0)
    return 17;
  if (loop_conditional_initializer(3, &counter) != 9)
    return 18;
  if (counter != 2)
    return 19;
  counter = 0;
  if (loop_conditional_initializer(7, &counter) != 19)
    return 20;
  if (counter != 3)
    return 21;

  // A zero-trip loop must not evaluate its dereferencing initializer.
  if (loop_load_initializer(0, 0) != 0)
    return 22;
  int value = 3;
  if (loop_load_initializer(3, &value) != 12)
    return 23;
  if (value != 3)
    return 24;
  return 0;
}
