// Runtime test: operator bool for container/handle state checks.
struct Handle {
  int fd;

  operator bool() {
    return fd >= 0;
  }
};

struct OptionalInt {
  int value;
  bool has_value;

  operator bool() {
    return has_value;
  }

  int get() {
    return value;
  }
};

int main() {
  Handle h1;
  h1.fd = 42;
  Handle h2;
  h2.fd = -1;

  if (!h1) return 1;       // h1 is valid
  if (h2) return 2;        // h2 is invalid

  OptionalInt opt;
  opt.value = 99;
  opt.has_value = true;

  if (!opt) return 3;
  if (opt.get() != 99) return 4;

  opt.has_value = false;
  if (opt) return 5;        // now empty

  // Use in conditional expressions
  int result = h1 ? 1 : 0;
  if (result != 1) return 6;

  result = h2 ? 1 : 0;
  if (result != 0) return 7;

  return 0;
}
