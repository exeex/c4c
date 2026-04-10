// HIR regression: template-struct argument materialization should keep
// defaulted type/value arguments and explicit value-driven materialization
// stable after the helper cluster moves behind its own translation unit.

template<typename T = short, int N = 2>
struct Holder {
  T data[N];
};

template<int N>
struct Count {
  static constexpr int value = N;
};

Holder<> default_holder;
Holder<int, Count<3>::value + 1> explicit_holder;

int main() {
  return sizeof(default_holder) + sizeof(explicit_holder);
}
