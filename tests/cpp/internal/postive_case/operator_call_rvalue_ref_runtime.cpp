// Runtime test: operator() dispatch should materialize rvalue-ref args.
struct Less {
  bool operator()(int&& a, int&& b) const {
    return a < b;
  }
};

int main() {
  Less less;
  return less(1, 2) - 1;
}
