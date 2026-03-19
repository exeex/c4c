// Negative test: namespace members should not leak into the global scope
// without qualification or a using declaration/directive.
namespace store {
int value = 21;
}

int main() {
  return value;
}
