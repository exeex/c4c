// Runtime test: namespace-scoped global variables should be accessible.
namespace config {
int value = 42;
int get_value() { return value; }
}

int main() {
  if (config::get_value() != 42) return 1;
  return 0;
}
