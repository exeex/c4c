// Expected: compile fail (case-local block variable used outside its block)
int main() {
  int x = 1;
  switch (x) {
    case 1: {
      int s = 42;
      break;
    }
    default:
      break;
  }
  return s;
}
