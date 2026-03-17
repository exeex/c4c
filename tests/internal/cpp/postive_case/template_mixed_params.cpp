// Test: mixed type + non-type template parameters.
// template<typename T, int N> where T is a type and N is an integer constant.

template <typename T, int N>
T add_n(T x) {
  return x + N;
}

template <typename T, int N>
T get_nth(T* arr) {
  return arr[N];
}

template <int N, typename T>
T scale(T x) {
  return x * N;
}

int main() {
  // Mixed: typename first, int second.
  if (add_n<int, 10>(5) != 15) return 1;
  if (add_n<int, 0>(7) != 7) return 2;
  if (add_n<long, 100>(200L) != 300) return 3;

  // Array element access via mixed template.
  int data[] = {10, 20, 30, 40};
  if (get_nth<int, 0>(data) != 10) return 4;
  if (get_nth<int, 2>(data) != 30) return 5;

  // int first, typename second.
  if (scale<3, int>(7) != 21) return 6;
  if (scale<2, long>(50L) != 100) return 7;

  return 0;
}
