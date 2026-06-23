struct PairBox {
  int lhs;
  union {
    int rhs;
    int alias;
  };
  int tail;
};

int main(void) {
  struct PairBox box;

  box.lhs = 5;
  box.rhs = 7;
  box.tail = 11;

  if (box.lhs != 5 && box.alias != 7)
    return 2;
  if (box.tail != 11)
    return 3;
  return 0;
}
