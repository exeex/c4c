enum { K_CASE = 1 };

int pick(int x) {
  {
    enum { K_CASE = 2 };
    if (x == K_CASE) return 20;
  }

  // Must resolve to global K_CASE == 1 after leaving the inner block.
  switch (x) {
    case K_CASE:
      return 10;
    case 2:
      return 20;
    default:
      return 0;
  }
}

int main(void) {
  return pick(1) == 10 ? 0 : 1;
}
