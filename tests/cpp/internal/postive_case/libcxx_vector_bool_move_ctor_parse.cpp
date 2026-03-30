// Reduced from libc++ __vector/vector_bool.h: templated out-of-class move
// constructor definitions should accept the injected-class-name parameter
// spelling `Owner&& other`.
// RUN: %c4cll --parse-only %s

template <class T, class Allocator>
struct Vec;

template <class Allocator>
struct Vec<bool, Allocator> {
  int storage;
  Vec(Vec&& other);
};

template <class Allocator>
inline __attribute__((__visibility__("hidden"))) constexpr Vec<bool,
                                                              Allocator>::Vec(
    Vec&& other) noexcept
    : storage(0) {}

int main() {
  return 0;
}
