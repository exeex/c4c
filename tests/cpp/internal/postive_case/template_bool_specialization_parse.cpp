// Parse test: bool NTTP specialization for class templates.
template <bool>
struct TruthType {
  typedef int type;
};

template <>
struct TruthType<true> {
  typedef long type;
};

int main() {
  return 0;
}
