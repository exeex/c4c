namespace lib {

template <typename Head, typename Tail>
struct owner {
  using selected = Head;
};

template <typename A, typename B>
using select_t = typename owner<B, A>::selected;

}  // namespace lib

template <typename Head, typename Tail>
struct owner {
  using selected = Head;
};

template <typename A, typename B>
using select_t = typename owner<A, B>::selected;

template <typename A, typename B>
struct is_same {
  static constexpr bool value = false;
};

template <typename A>
struct is_same<A, A> {
  static constexpr bool value = true;
};

template <typename A, typename B>
constexpr bool is_same_v = is_same<A, B>::value;

int main() {
  if (!is_same_v<lib::select_t<int, short>, short>)
    return 1;
  if (!is_same_v<::select_t<int, short>, int>)
    return 2;
  return 0;
}
