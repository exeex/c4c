namespace ns {

template <typename T>
struct holder {
  using type = T;
};

template <typename T>
struct is_signed {
  static constexpr bool value = false;
};

template <>
struct is_signed<int> {
  static constexpr bool value = true;
};

template <typename T>
constexpr bool is_signed_v = is_signed<T>::value;

template <typename T, typename U>
struct is_same {
  static constexpr bool value = false;
};

template <typename T>
struct is_same<T, T> {
  static constexpr bool value = true;
};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

}  // namespace ns

int main() {
  if (!ns::is_signed<typename ns::holder<int>::type>::value)
    return 1;
  if (!ns::is_signed_v<typename ns::holder<int>::type>)
    return 2;
  if (!ns::is_same_v<typename ns::holder<int>::type, int>)
    return 3;
  return 0;
}
