// Expected: compile fail (duplicate case label)
int main() {
  int x = 1;
  switch (x) {
    case 1: return 1;
    case 1: return 2;
    default: return 0;
  }
}
