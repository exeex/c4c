// Negative test: calling a non-consteval (runtime-only) function from
// within a consteval function body must be a compile error.

int runtime_fn(int x) {
  return x + 1;
}

consteval int bad(int x) {
  return runtime_fn(x);  // error: runtime_fn is not consteval
}

int main() {
  int r = bad(5);
  return r;
}
