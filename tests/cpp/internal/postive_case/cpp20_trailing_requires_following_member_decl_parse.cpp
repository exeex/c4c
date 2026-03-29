// Reduced parser-boundary regression: even if the preceding member declaration
// is malformed, a trailing requires-clause must stop at the end of its
// constraint-expression so the following member declaration is still visible.
template<typename T>
struct box {
  int f() requires (sizeof(T) > 0)
  struct inner {};
};
