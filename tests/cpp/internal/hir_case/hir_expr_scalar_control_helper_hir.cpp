// HIR regression: scalar/control helpers extracted from impl/expr/expr.cpp must
// preserve implicit field lookup, scalar operator lowering, GNU statement
// expressions, and complex real/imag access.

struct Acc {
  static const int bias = 5;
  int base;

  int run(int input) {
    int local = input + bias;
    local += base;
    int picked = local > 6 ? ({ int t = local; t - 1; }) : ({ int f = bias; f + 2; });

    __complex__ double z;
    __real__ z = 3.0;
    __imag__ z = 4.0;

    int values[3] = {1, 2, 3};
    return picked + (int)__real__ z + (int)__imag__ z + values[1] + (int)sizeof(values[0]);
  }
};

int main() {
  Acc acc;
  acc.base = 2;
  return acc.run(1);
}
