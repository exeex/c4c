// Reduced frontend regression from libstdc++ ranges_util.h: concept-id
// conditions in if constexpr should fold as constant booleans instead of
// lowering as untyped declrefs.

template<typename T>
concept bidirectional_iterator = true;

template<typename T>
int classify() {
  if constexpr (bidirectional_iterator<T>) {
    return 1;
  }
  return 0;
}

int main() {
  return classify<int>() - 1;
}
