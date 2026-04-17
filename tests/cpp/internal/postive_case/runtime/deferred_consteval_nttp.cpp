// Test: consteval with NTTP inside deferred template instantiation.
//
// Chain: main → outer<5> → compute_square<5> → inner_square<5> (consteval)
//   and: main → wrapper<3, 10> → deferred_sum<3, 10> → sum_nttp<3, 10> (consteval)
//
// outer<5> is a depth-0 template call.
// compute_square<5> is deferred (nested template-to-template call).
// inner_square<5> is a consteval NTTP call inside the deferred function.
//
// wrapper<3, 10> is a depth-0 template call with multiple NTTPs.
// deferred_sum<3, 10> is deferred and calls consteval with forwarded NTTPs.

// Pure NTTP consteval, called from inside a deferred template.
template <int N>
consteval int inner_square() {
  return N * N;
}

// Non-consteval template that calls consteval NTTP function.
// This will be instantiated by the deferred pass.
template <int N>
int compute_square() {
  return inner_square<N>();
}

// Depth-0 entry: triggers deferred instantiation of compute_square<N>.
template <int N>
int outer() {
  return compute_square<N>();
}

// Multiple NTTP consteval, called from deferred chain.
template <int A, int B>
consteval int sum_nttp() {
  return A + B;
}

// Deferred: multiple NTTPs, calls consteval with forwarded NTTPs.
template <int A, int B>
int deferred_sum() {
  return sum_nttp<A, B>();
}

// Depth-0 entry for multiple NTTP chain.
template <int A, int B>
int wrapper() {
  return deferred_sum<A, B>();
}

int main() {
  // Pure NTTP through deferred chain.
  int a = outer<5>();
  if (a != 25) return 1;

  int b = outer<7>();
  if (b != 49) return 2;

  // Multiple NTTP through deferred chain.
  int c = wrapper<3, 10>();
  if (c != 13) return 3;

  int d = wrapper<100, 200>();
  if (d != 300) return 4;

  return 0;
}
