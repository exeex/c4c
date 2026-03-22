// Runtime test: templated struct members should keep template params visible
// when constexpr/consteval appear before the return type.
#define EA_CPP14_CONSTEXPR constexpr

struct S {
  template<typename A>
  EA_CPP14_CONSTEXPR auto f(A a) -> int {
    return a;
  }
};

int main() {
  S s;
  return s.f(7) - 7;
}
