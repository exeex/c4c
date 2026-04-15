int accumulate_vla(int n) {
  int total = 0;
loop:
  {
    int xs[n];
    xs[0] = n;
    total += xs[0];
  }
  n = n - 1;
  if (n > 0) goto loop;
  return total;
}

int main(void) {
  return accumulate_vla(3);
}
