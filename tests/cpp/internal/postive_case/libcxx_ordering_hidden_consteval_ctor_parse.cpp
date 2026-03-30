// Parse-only regression: libc++ ordering constructors can carry a leading GNU
// attribute before the constructor name and a trailing __enable_if__ attribute
// after the signature.
// RUN: %c4cll --parse-only %s

template <class T>
using __enable_if_t = int;

template <class A, class B>
constexpr bool is_same_v = false;

template <class A>
constexpr bool is_same_v<A, int> = true;

struct _CmpUnspecifiedParam {
  template <class _Tp, class = __enable_if_t<!is_same_v<_Tp, int> > >
  __attribute__((visibility("hidden"))) consteval _CmpUnspecifiedParam(
      _Tp __zero) noexcept
      __attribute__((__enable_if__(
          __zero == 0, "Only literal 0 is allowed")))
  {
    (void)__zero;
  }
};

int main() {
  return 0;
}
