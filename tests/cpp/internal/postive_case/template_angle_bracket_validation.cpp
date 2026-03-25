// Validation test for clang-style angle bracket parsing plan focus corpus.
// Tests: nested templates, dependent NTTPs, qualified ::type/::value,
// template-vs-expression ambiguity, parenthesized comparison in NTTP.

// --- Nested templates: A<B<C>> ---
template <typename T> struct Wrap { T val; };

Wrap<Wrap<int>> nested2() {
  Wrap<Wrap<int>> w;
  w.val.val = 42;
  return w;
}

// --- Deeply nested: A<B<C<D>>> ---
Wrap<Wrap<Wrap<int>>> nested3() {
  Wrap<Wrap<Wrap<int>>> w;
  w.val.val.val = 99;
  return w;
}

// --- integral_constant and Trait<T>::value NTTP ---
template <typename T, T v>
struct integral_constant {
  static constexpr T value = v;
};

template <typename T>
struct is_pointer : integral_constant<bool, false> {};

template <typename T>
struct is_pointer<T*> : integral_constant<bool, true> {};

// --- enable_if_t<(N > 0), int> style parenthesized comparison ---
template <bool Cond, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { using type = T; };

template <bool Cond, typename T = void>
using enable_if_t = typename enable_if<Cond, T>::type;

template <int N>
enable_if_t<(N > 0), int> positive_only() { return N; }

// --- foo<bar, baz>(x) template-id in expression context ---
template <typename T, typename U>
int convert(T x) { return static_cast<int>(x); }

int main() {
  // Nested templates: A<B<C>>
  Wrap<Wrap<int>> w2 = nested2();
  if (w2.val.val != 42) return 1;

  // Deeply nested: A<B<C<D>>>
  Wrap<Wrap<Wrap<int>>> w3 = nested3();
  if (w3.val.val.val != 99) return 2;

  // Trait<T>::value NTTP
  if (is_pointer<int>::value != false) return 3;
  if (is_pointer<int*>::value != true) return 4;

  // enable_if_t<(N > 0), int> — parenthesized comparison in NTTP
  int r = positive_only<5>();
  if (r != 5) return 5;

  // foo<bar, baz>(x) — template-id in expression context
  long val = 100L;
  int c = convert<long, int>(val);
  if (c != 100) return 6;

  return 0;
}
