// Candidate:
// - multiple consteval steps depend on a late-completed type
// - this is closer to NN compiler shape / vector-lane inference
//
// c4c angle:
// - pending compile-time values should propagate through the whole chain

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

static_assert(cost == 24);

int main() {
  return 0;
}
