int inc(int x) { return x + 1; }

int (*gp)(int) = inc;

int main(void) {
  int (*p)(int) = gp;
  return p(4);
}
