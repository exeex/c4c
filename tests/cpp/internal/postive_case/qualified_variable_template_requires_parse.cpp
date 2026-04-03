// Parse-only regression: qualified `_v` variable-template expressions inside
// requires clauses should go straight to expression parsing instead of paying
// the expensive speculative type probe first.
// RUN: %c4cll --parse-only %s

namespace std {
template <typename T>
inline constexpr bool is_reference_v = false;
}

template <typename T>
struct box {
  void use() requires (!std::is_reference_v<T>);
};

template <typename T>
void box<T>::use() requires (!std::is_reference_v<T>) {}

int main() {
  box<int> value;
  value.use();
  return 0;
}
