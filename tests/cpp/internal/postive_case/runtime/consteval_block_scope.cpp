// Test: consteval block scope should restore shadowed locals after nested blocks.

consteval int shadow_keeps_outer() {
  int x = 1;
  {
    int x = 2;
    if (x != 2) return -1;
  }
  return x;
}

consteval int outer_assignment_persists() {
  int x = 3;
  {
    x = x + 4;
  }
  return x;
}

int main() {
  int a = shadow_keeps_outer();
  if (a != 1) return 1;

  int b = outer_assignment_persists();
  if (b != 7) return 2;

  return 0;
}
