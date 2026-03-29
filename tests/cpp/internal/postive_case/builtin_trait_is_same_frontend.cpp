// Reduced semantic regression for the first audited builtin trait slice.
// __is_same must distinguish matching and mismatched type names at frontend
// constant-evaluation time.

static_assert(__is_same(int, int));
static_assert(!__is_same(int, long));

int main() {
  return __is_same(int, long) ? 1 : 0;
}
