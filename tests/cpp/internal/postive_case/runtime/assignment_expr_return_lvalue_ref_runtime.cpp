// Regression: assignment expressions remain lvalues, so a function returning
// `T&` can return `lhs = rhs` directly.

int& assign_and_return(int& slot, int value) {
  return slot = value;
}

int main() {
  int storage = 1;

  int& alias = assign_and_return(storage, 7);
  if (&alias != &storage) return 1;
  if (storage != 7) return 2;

  alias = 11;
  if (storage != 11) return 3;

  return 0;
}
