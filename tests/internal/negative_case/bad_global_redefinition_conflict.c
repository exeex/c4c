// Expected: compile fail (global initializer uses undeclared symbol)
int g = missing_symbol + 1;

int main(void) {
  return g;
}
