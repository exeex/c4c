short *rv64_advance_and_store(short *base, int shift, int *out) {
  short narrowed;

  narrowed = *base;
  base++;
  narrowed = shift << narrowed;
  *out = narrowed;
  return base;
}

int main(void) {
  short values[2];
  int stored;
  short *advanced;

  values[0] = 0;
  values[1] = 9;
  stored = 0;

  advanced = rv64_advance_and_store(values, 1, &stored);
  if (advanced != &values[1]) {
    return 1;
  }
  if (stored != 1) {
    return 2;
  }
  return 0;
}
