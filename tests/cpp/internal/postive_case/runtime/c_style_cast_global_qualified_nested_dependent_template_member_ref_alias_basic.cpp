int pick(int&) { return 1; }
int pick(int&&) { return 2; }

namespace ns {
template <class T>
struct Holder {
  template <class U>
  struct Rebind {
    struct Inner {
      using AliasL = U&;
      using AliasR = U&&;
    };
  };
};
}

template <class T>
int probe(T value) {
  int x = value;

  typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasL l =
      (typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasL)x;
  int lkind =
      pick((typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasL)x);
  int rkind =
      pick((typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasR)x);
  typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasR r =
      (typename ::ns::Holder<T>::template Rebind<T>::Inner::AliasR)x;

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
