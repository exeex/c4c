// HIR-only case:
// late-known layout information should flow through multiple consteval steps.

template <class T>
consteval int bytes() {
  return sizeof(T);
}

template <class T>
consteval int lanes() {
  return bytes<T>() / 4;
}

template <class T>
consteval int vec_cost() {
  return lanes<T>() * 3;
}

struct F32x8;
constexpr int cost = vec_cost<F32x8>();

struct F32x8 {
  float data[8];
};

int main() {
  return cost == 24 ? 0 : 1;
}
