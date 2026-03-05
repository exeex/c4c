// Expected: compile fail (unknown type in cast)
int main() {
  int x = 1;
  return (NotAType)x;
}
