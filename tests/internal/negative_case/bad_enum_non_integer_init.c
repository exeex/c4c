// Expected: compile fail (enum initializer not integer constant expression)
int main() {
  int x = 3;
  enum E {
    A = x
  };
  return A;
}
