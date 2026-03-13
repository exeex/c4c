// Expected: compile fail (duplicate field name in union)
union U {
  int x;
  char x;
};

int main() {
  return 0;
}
