// Expected: compile fail (increment const object)
int main() {
  const int x = 1;
  ++x;
  return x;
}
