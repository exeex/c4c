// Parse test: namespace-scope using alias and aliased type use.
using IntAlias = int;

int main() {
  IntAlias value = 5;
  return value - 5;
}
