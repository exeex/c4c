// Reduced from libstdc++ iterator_concepts.h: a declaration-level requires
// clause on a template must stop before the following class-key declaration.
// RUN: %c4cll --parse-only %s

template<typename T>
constexpr bool is_object_v = true;

template<typename T>
requires is_object_v<T>
struct trait;

int main() {
  return 0;
}
