// Regression: forwarding-reference wrappers must deduce prvalue call
// expressions and preserve ref-qualified member dispatch.

struct Probe {
  int select() & { return 1; }
  int select() && { return 2; }
};

Probe make() { return Probe{}; }

template <class T>
int call_select(T&& t) {
  return static_cast<T&&>(t).select();
}

int main() {
  Probe p;

  if (call_select(p) != 1) return 1;
  if (call_select(make()) != 2) return 2;

  return 0;
}
