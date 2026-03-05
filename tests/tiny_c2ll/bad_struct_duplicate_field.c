// Expected: compile fail (duplicate field name in struct)
struct S {
  int x;
  int x;
};

int main() {
  return 0;
}
