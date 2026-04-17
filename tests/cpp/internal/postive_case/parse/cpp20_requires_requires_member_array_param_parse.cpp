// Reduced repro for member-template requires-clause parsing where the
// constraint-expression is itself a requires-expression and the following
// member declaration uses array-reference parameters.
using size_t = unsigned long;

struct Swap {
  template<class T, class U, size_t N>
  requires requires(const Swap& s, T& a, U& b) {
    s(a, b);
  }
  void apply(T (&a)[N], U (&b)[N]) const;
};

int main() {
  return 0;
}
