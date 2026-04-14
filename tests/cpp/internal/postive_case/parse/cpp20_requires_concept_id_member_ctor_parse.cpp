template<class T>
concept __convertible = true;

template<class T>
struct reverse_iterator {
  template<class Iter>
  requires __convertible<Iter>
  constexpr reverse_iterator(const reverse_iterator<Iter>& x) {}
};

int main() {
  return 0;
}
