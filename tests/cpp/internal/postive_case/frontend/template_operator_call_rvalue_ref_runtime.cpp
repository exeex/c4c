// Runtime test: templated operator() should handle rvalue-ref args.
template<typename T>
struct less_void {
  template<typename A, typename B>
  bool operator()(A&& a, B&& b) const {
    return a < b;
  }
};

int main() {
  less_void<int> less;
  return less(1, 2) - 1;
}
