// Baseline: nested template consteval call that mainstream compilers already
// accept. This is a control case for the stronger deferred-consteval patterns.

template <int I>
consteval int foo(int j) {
  return I - j;
}

template <int I>
consteval int bar() {
  return foo<I + 1>(3);
}

static_assert(bar<5>() == 3);

int main() {
  return 0;
}
