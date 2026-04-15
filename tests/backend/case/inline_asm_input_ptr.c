int touch(int *p) {
  asm volatile("" : : "r"(p));
  return 0;
}

int main(void) {
  int x = 7;
  return touch(&x);
}
