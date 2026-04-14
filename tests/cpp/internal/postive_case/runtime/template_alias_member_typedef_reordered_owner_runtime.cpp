namespace ns {

template <typename Head, typename Tail = void>
struct pick_first {
  using type = Head;
};

template <typename A, typename B = long>
using choose_second_t = typename pick_first<B, A>::type;

}  // namespace ns

int main() {
  ns::choose_second_t<int, short> value = 7;
  return value - 7;
}
