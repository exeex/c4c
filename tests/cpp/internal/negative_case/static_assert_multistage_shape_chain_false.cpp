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

// This depends on the same late-layout-driven reduction chain as the positive
// c4-only case, but should fail once the engine resolves the actual value.
static_assert(cost == 12, "multistage deferred shape chain should fail after engine resolution");

int main() {
  return 0;
}
