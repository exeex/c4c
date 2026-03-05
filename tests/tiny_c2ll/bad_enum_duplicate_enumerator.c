// Expected: compile fail (duplicate enumerator name)
enum E {
  A = 1,
  A = 2
};

int main() {
  return A;
}
