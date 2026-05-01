namespace carrier_probe {

template <typename Head, typename Tail>
struct owner {
  using selected = Tail;
};

template <typename A, typename B>
using pick_second_t = typename owner<B, A>::selected;

}  // namespace carrier_probe

int main() {
  carrier_probe::pick_second_t<int, short> value = 9;
  return value - 9;
}
