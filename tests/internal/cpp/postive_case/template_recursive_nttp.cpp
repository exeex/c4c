// Test: deeper function-template specialization chains.
// This avoids unsupported NTTP expressions like N-1 while still exercising
// repeated template-to-template calls and specialization selection.

template <int N>
int ladder() {
  return -1;
}

template <>
int ladder<0>() {
  return 42;
}

template <>
int ladder<1>() {
  return ladder<0>();
}

template <>
int ladder<2>() {
  return ladder<1>();
}

template <>
int ladder<3>() {
  return ladder<2>();
}

template <>
int ladder<4>() {
  return ladder<3>();
}

template <int N>
int offset_ladder(int x) {
  return ladder<N>() + x;
}

template <>
int offset_ladder<0>(int x) {
  return x;
}

template <>
int offset_ladder<1>(int x) {
  return offset_ladder<0>(x + 1);
}

template <>
int offset_ladder<2>(int x) {
  return offset_ladder<1>(x + 1);
}

template <>
int offset_ladder<3>(int x) {
  return offset_ladder<2>(x + 1);
}

template <>
int offset_ladder<4>(int x) {
  return offset_ladder<3>(x + 1);
}

template <>
int offset_ladder<5>(int x) {
  return offset_ladder<4>(x + 1);
}

int main() {
  if (ladder<0>() != 42) return 1;
  if (ladder<4>() != 42) return 2;
  if (offset_ladder<0>(42) != 42) return 3;
  if (offset_ladder<5>(37) != 42) return 4;

  return 0;
}
