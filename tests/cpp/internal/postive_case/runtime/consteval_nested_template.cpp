// Test: consteval function called from inside a non-consteval template function.
// The consteval call's template args must be resolved through the enclosing
// template context before interpretation.

template <typename T>
consteval int get_size() {
  return sizeof(T);
}

template <typename T>
consteval bool is_large() {
  return sizeof(T) >= 8;
}

// Non-consteval template calling consteval template.
template <typename T>
int size_via_consteval() {
  return get_size<T>();
}

// Non-consteval template using consteval in if constexpr.
template <typename T>
int classify_size() {
  if constexpr (is_large<T>()) {
    return 8;
  } else {
    return 4;
  }
}

// Chained: non-consteval template calling consteval which calls consteval.
template <typename T>
consteval int double_size() {
  return get_size<T>() * 2;
}

template <typename T>
int get_double_size() {
  return double_size<T>();
}

int main() {
  // Direct consteval template call (already works).
  if (get_size<int>() != 4) return 1;
  if (get_size<long>() != 8) return 2;

  // Consteval called from non-consteval template.
  if (size_via_consteval<int>() != 4) return 3;
  if (size_via_consteval<long>() != 8) return 4;

  // Consteval in if constexpr inside non-consteval template.
  if (classify_size<int>() != 4) return 5;
  if (classify_size<long>() != 8) return 6;

  // Chained consteval from non-consteval template.
  if (get_double_size<int>() != 8) return 7;
  if (get_double_size<long>() != 16) return 8;

  return 0;
}
