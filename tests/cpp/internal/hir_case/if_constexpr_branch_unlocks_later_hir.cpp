// HIR-only case:
// branch selection depends on a type that becomes complete later in the TU.

template <class T>
consteval bool use_wide_path() {
  return sizeof(T) >= 32;
}

template <class T>
consteval int choose_tile() {
  if constexpr (use_wide_path<T>()) {
    return 128;
  } else {
    return 32;
  }
}

struct TileShape;
constexpr int tile = choose_tile<TileShape>();

struct TileShape {
  int x[8];
};

int main() {
  return tile == 128 ? 0 : 1;
}
