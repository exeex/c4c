// Expected: compile fail (discarding const qualifier in local static pointer initialization)
const int cx = 1;

int main(void) {
  static int *p = &cx;
  return p != 0;
}
