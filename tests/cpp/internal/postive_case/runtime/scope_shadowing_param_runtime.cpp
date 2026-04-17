// Runtime test: local shadowing should prefer the innermost binding while
// leaving the parameter value unchanged outside the nested block.
int pick_value(int value) {
  int total = value;

  {
    int value = 9;
    total += value;
  }

  total += value;
  return total;
}

int main() {
  int result = pick_value(4);
  if (result != 17) return 1;
  return 0;
}
