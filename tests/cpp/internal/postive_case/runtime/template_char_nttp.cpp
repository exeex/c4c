// Test: char NTTP arguments in function templates.

template <char C>
int code() {
  return (int)C;
}

template <char C>
int offset(int x) {
  return x + (int)C;
}

int main() {
  if (code<'A'>() != 65) return 1;
  if (code<'*'>() != 42) return 2;
  if (code<'\n'>() != 10) return 3;

  if (offset<'A'>(-23) != 42) return 4;
  if (offset<'*'>(0) != 42) return 5;

  return 0;
}
