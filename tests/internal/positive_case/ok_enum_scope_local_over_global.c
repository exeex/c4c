enum { E_CASE = 4 };

int pick(int x) {
  enum { E_CASE = 5 };
  switch (x) {
    case E_CASE:
      return 50;
    case 4:
      return 40;
    default:
      return 0;
  }
}

int main(void) {
  return pick(5) == 50 ? 0 : 1;
}
