// Positive case extracted from consteval_case:
// a late explicit specialization should unlock a deferred consteval call once
// the wrapper template is instantiated.

template <int Op>
consteval int cost();

template <int Op>
consteval int schedule() {
  return cost<Op>() + 1;
}

template <int Op>
int wrapper() {
  return schedule<Op>();
}

template <>
consteval int cost<7>() {
  return 41;
}

int main() {
  int result = wrapper<7>();
  return result == 42 ? 0 : 1;
}
