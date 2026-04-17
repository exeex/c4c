// Parse-only regression: functional-cast arguments should not misclassify a
// qualified template-id call as a plain qualified variable.
// RUN: %c4cll --parse-only %s

namespace std {
template <typename T>
T&& declval() noexcept;
}

template <typename T>
struct Box {
  static constexpr bool check() {
    return noexcept(bool(std::declval<T&>().empty()));
  }
};

int main() {
  return 0;
}
