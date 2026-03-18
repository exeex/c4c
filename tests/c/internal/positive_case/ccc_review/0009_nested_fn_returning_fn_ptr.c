// CCC_EXPECT: pass

int plus1(int x) { return x + 1; }
int minus1(int x) { return x - 1; }

int (*pick_inner(int which))(int) {
  return which ? plus1 : minus1;
}

int (*(*pick_outer(int which))(int))(int) {
  return pick_inner;
}

int main(void) {
  return pick_outer(0)(1)(41) == 42 ? 0 : 1;
}
