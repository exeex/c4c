int pick(int&) { return 1; }
int pick(int&&) { return 2; }

template <class T>
struct Holder {
  struct Inner {
    using AliasL = T&;
    using AliasR = T&&;
  };
};

template <class T>
int probe(T value) {
  int x = value;

  typename Holder<T>::Inner::AliasL l = (typename Holder<T>::Inner::AliasL)x;
  int lkind = pick((typename Holder<T>::Inner::AliasL)x);
  int rkind = pick((typename Holder<T>::Inner::AliasR)x);
  typename Holder<T>::Inner::AliasR r = (typename Holder<T>::Inner::AliasR)x;

  l = value + 4;
  r = value + 8;

  return (x == value + 8 && l == value + 8 && r == value + 8 && lkind == 1 &&
          rkind == 2)
             ? 0
             : 1;
}

int main() {
  return probe<int>(3);
}
