// Test: mixed type+NTTP forwarding in deferred template instantiation chains.
//
// Chain: main → outer<int, 5> → compute<int, 5> → inner<int, 5> (consteval)
//   and: main → apply<int, 3, 10> → deferred_op<int, 3, 10> → sum_typed<int, 3, 10> (consteval)
//
// This tests forwarding both typename T and int N through deferred chains.
// The type parameter T is carried through for template identity/mangling,
// while NTTP values drive the consteval computation.

// Consteval with mixed type+NTTP params.
template <typename T, int N>
consteval int inner() {
  return N * N;
}

// Non-consteval template forwarding both T and N.
template <typename T, int N>
int compute() {
  return inner<T, N>();
}

// Depth-0 entry: triggers deferred instantiation of compute<T, N>.
template <typename T, int N>
int outer() {
  return compute<T, N>();
}

// Multiple NTTPs with type forwarding.
template <typename T, int A, int B>
consteval int sum_typed() {
  return A + B;
}

template <typename T, int A, int B>
int deferred_op() {
  return sum_typed<T, A, B>();
}

template <typename T, int A, int B>
int apply() {
  return deferred_op<T, A, B>();
}

// Type-only forwarding alongside NTTP: different T produces different
// instantiation even when N is the same.
template <typename T, int N>
consteval int type_tag() {
  return N + 1;
}

template <typename T, int N>
int tagged() {
  return type_tag<T, N>();
}

int main() {
  // Mixed type+NTTP through deferred chain.
  int a = outer<int, 5>();
  if (a != 25) return 1;

  int b = outer<long, 7>();
  if (b != 49) return 2;

  // Multiple NTTPs with type forwarding.
  int c = apply<long, 3, 10>();
  if (c != 13) return 3;

  int d = apply<int, 100, 200>();
  if (d != 300) return 4;

  // Same NTTP, different type → different instantiation.
  int e = tagged<int, 10>();
  if (e != 11) return 5;

  int f = tagged<long, 10>();
  if (f != 11) return 6;

  return 0;
}
