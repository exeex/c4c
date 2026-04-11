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

// This only becomes decidable after late layout information reaches the
// compile-time engine. The assertion should then fail there, not succeed.
static_assert(tile == 32, "late branch unlock should fail after engine resolution");

int main() {
  return 0;
}
