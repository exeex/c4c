template<class T>
concept __convertible = true;

template<class A, class B>
concept assignable_from = true;

struct Box {
  template<class Iter>
  requires __convertible<Iter> && assignable_from<int&, const Iter&>
  void assign(Iter x) const {}
};

int main() {
  return 0;
}
