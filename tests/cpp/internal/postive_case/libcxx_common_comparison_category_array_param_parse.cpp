// Parse-only regression: libc++ common comparison category computes over an
// array-reference parameter of an injected enum type.
// RUN: %c4cll --parse-only %s

using size_t = unsigned long;

enum _ClassifyCompCategory {
  _None,
  _PartialOrd,
  _WeakOrd,
  _StrongOrd,
  _CCC_Size,
};

template <size_t _Size>
constexpr _ClassifyCompCategory __compute_comp_type(
    const _ClassifyCompCategory (&__types)[_Size]) {
  return _StrongOrd;
}

int main() {
  return 0;
}
