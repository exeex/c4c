// Parse test: template parameter packs in declarators like Args&&... args.
template<typename F, typename... Args>
auto invoke(F&& func, Args&&... args) -> decltype(func(args...)) {
  return func(args...);
}

int add3(int a, int b, int c) {
  return a + b + c;
}

int main() {
  return invoke(add3, 1, 2, 3) - 6;
}
