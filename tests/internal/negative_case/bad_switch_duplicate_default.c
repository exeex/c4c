// Expected: compile fail (duplicate default label)
int main() {
  int x = 1;
  switch (x) {
    default: return 1;
    default: return 2;
  }
}
