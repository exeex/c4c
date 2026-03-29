// Parse-only regression: dependent typename-qualified member lookups must
// accept an optional `template` keyword before the member name.
// RUN: %c4cll --parse-only %s

template<bool>
struct conditional_impl {
  template<typename T, typename>
  using type = T;
};

template<>
struct conditional_impl<false> {
  template<typename, typename U>
  using type = U;
};

template<bool Cond, typename If, typename Else>
using conditional_t =
    typename conditional_impl<Cond>::template type<If, Else>;

conditional_t<true, int, long> pick_if();

int main() {
  return 0;
}
