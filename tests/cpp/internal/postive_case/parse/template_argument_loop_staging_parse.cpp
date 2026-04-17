// Parse-only regression: keep the canonical template-argument loop stable while
// it is compressed into a helper-driven coordinator.
// RUN: %c4cll --parse-only %s

template <typename T>
struct wrap {};

template <typename Left, bool Flag, typename Right>
struct triad {};

template <typename T>
using selected = triad<T, T(-1) < T(0), wrap<T>>;

int main() {
  selected<int>* value = nullptr;
  return value != nullptr;
}
