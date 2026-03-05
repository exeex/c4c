// Expected: compile fail (variable declared in while block used outside)
int main() {
  while (1) {
    int t = 7;
    break;
  }
  return t;
}
