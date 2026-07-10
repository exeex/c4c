void writeback_callee(int *base, int **cursor) {
  int i;

  for (i = 0; i < 3; i = i + 1) {
    *cursor++ = &base[i];
  }
}

int main(void) {
  int values[3];
  int *slots[3];

  slots[0] = 0;
  slots[1] = 0;
  slots[2] = 0;
  writeback_callee(values, slots);

  if (slots[2] != &values[2]) {
    return 1;
  }
  return 0;
}
