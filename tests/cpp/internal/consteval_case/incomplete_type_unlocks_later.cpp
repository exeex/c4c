// Candidate:
// - box<T> needs a default NTTP value from sz<T>()
// - sz<T>() needs sizeof(T)
// - T becomes complete later in the translation unit
//
// Mainstream C++:
// - rejects immediately because sizeof(T) is requested while T is incomplete
//
// c4c angle:
// - record a pending consteval reduction
// - revisit after the full record layout is known

template <class T>
consteval int sz() {
  return sizeof(T);
}

template <class T, int N = sz<T>()>
struct box {
  static constexpr int value = N;
};

struct TensorDesc;
constexpr int bytes = box<TensorDesc>::value;

struct TensorDesc {
  int lanes[16];
};

static_assert(bytes == 64);

int main() {
  return 0;
}
