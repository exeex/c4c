// Reduced from libstdc++ bits/stl_iterator.h: allow requires-expressions in
// if constexpr conditions to parse as unevaluated boolean constraints.

namespace std {
struct forward_iterator_tag {};
struct input_iterator_tag {};

template <typename T, typename U>
concept derived_from = true;

template <typename T>
using __iter_category_t = T;

template <typename _It>
struct iter_probe {
  static auto select() {
    if constexpr (requires { requires derived_from<__iter_category_t<_It>,
                                                   forward_iterator_tag>; })
      return forward_iterator_tag{};
    else
      return input_iterator_tag{};
  }
};
}
