// Expected: compile fail (undefined symbol in expression)
int main() {
  return missing_symbol + 1;
}
