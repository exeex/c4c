// Parse-only regression: expression-style non-type template arguments should
// still be accepted when they end immediately at the template close while the
// NTTP fallback path is being helperized.

template <int Value>
struct flag {};

template <typename T>
using ordered_flag = flag<(T(-1) < T(0))>;

int main() {
  ordered_flag<int>* value = nullptr;
  return value != nullptr;
}
