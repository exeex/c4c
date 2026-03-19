// Negative test: an alias name should not be usable before its using
// declaration appears.
int main() {
  Alias value = 0;
  return value;
}

using Alias = int;
