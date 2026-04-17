// Runtime regression: alias-template application over a non-empty type pack
// must still preserve the aliased concrete type instead of forcing a `_t`
// alias through deferred `owner::type` recovery.

template <typename... Ts>
using void_t = void;

template <typename T>
struct is_void {
  static constexpr int value = 0;
};

template <>
struct is_void<void> {
  static constexpr int value = 1;
};

int main() {
  if (!is_void<void_t<int, long, char>>::value)
    return 1;
  if (!is_void<void_t<int>>::value)
    return 2;
  return 0;
}
