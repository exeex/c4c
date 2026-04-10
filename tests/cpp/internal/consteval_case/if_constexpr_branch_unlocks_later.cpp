// Candidate:
// - branch selection itself depends on a late-known type property
// - useful for tile-size or path-selection logic in compile-time lowering

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

static_assert(tile == 128);

int main() {
  return 0;
}
