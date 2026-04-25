// HIR regression: switch lowering should keep the emitted case/default block
// structure stable across helper extraction from impl/stmt/stmt.cpp.

int classify(int x) {
  constexpr int first = 1;
  constexpr int second = first + 1;
  int result = -1;
  switch (x) {
    case first:
      result = 10;
      break;
    case second:
      result = 20;
      break;
    default:
      result = 30;
      break;
  }
  return result;
}

int main() {
  return classify(2) - 20;
}
