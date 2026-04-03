typedef struct Pair {
  double d;
  int i[3];
} Pair;

typedef struct Result {
  char buf[33];
  char tag;
} Result;

Result make_result(Pair left, char tag, double mid, Pair right) {
  Result out = {{0}, 0};
  out.tag = tag;
  return out;
}

Result (*fp)(Pair, char, double, Pair) = &make_result;

int main(void) {
  Pair a = {1.0, {1, 2, 3}};
  Pair b = {2.0, {4, 5, 6}};
  Result out = (*fp)(a, 7, 3.0, b);
  return out.tag;
}
