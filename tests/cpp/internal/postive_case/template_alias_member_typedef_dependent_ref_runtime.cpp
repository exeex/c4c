namespace ns {

template <typename T>
struct remove_ref {
  using type = T;
};

template <typename T>
struct remove_ref<T&> {
  using type = T;
};

template <typename T>
using remove_ref_t = typename remove_ref<T>::type;

template <typename A, typename B>
using alias_of_alias_t = remove_ref_t<B&>;

}  // namespace ns

int main() {
  ns::alias_of_alias_t<int, short> value = 7;
  return value - 7;
}
