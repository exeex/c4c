// Parse test: qualified names in expression context.
struct Traits {
  enum { value = 1 };
};

int main() {
  return bool(Traits::value);
}
