// Expected: compile fail (redeclare parameter in same function scope)
int f(int a) {
  int a = 2;
  return a;
}

int main() {
  return f(1);
}
