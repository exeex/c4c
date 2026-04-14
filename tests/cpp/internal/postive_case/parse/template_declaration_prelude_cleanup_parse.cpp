// Parse-only regression: declaration-template type parameters should stay
// visible for later parameters and the templated declaration body, then leave
// scope cleanly so a following ordinary declaration can reuse the same name.
// RUN: %c4cll --parse-only %s

template <typename T, typename U = T>
using Alias = U;

struct T {
  int value;
};

template <typename T = int>
struct Holder {
  T value;
};

int main() {
  Alias<int> alias_value = 1;
  Holder<> holder{2};
  T obj{3};
  return alias_value + holder.value + obj.value;
}
