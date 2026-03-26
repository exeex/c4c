// Parse test: dependent enum initializer with ternary + sizeof(type) should defer.
template <typename T>
struct Traits {
  enum { width = sizeof(T) ? sizeof(T) * 8 : 0 };
};

int main() {
  return 0;
}
