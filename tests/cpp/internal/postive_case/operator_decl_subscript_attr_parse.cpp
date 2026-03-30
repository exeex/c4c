// Parse-only regression: member operator[] can carry attributes before the
// parameter list, as seen in libstdc++ ranges CPO declarations.
// RUN: %c4cll --parse-only %s

struct Fn {
  template <typename T>
  constexpr auto operator()[[nodiscard]](T&& value) const {
    return value;
  }
};

int main() {
  return 0;
}
