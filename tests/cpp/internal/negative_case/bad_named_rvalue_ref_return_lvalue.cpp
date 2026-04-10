// Negative regression: a named `T&&` parameter is still an lvalue expression,
// so returning it directly from a `T&&` function must be rejected unless it is
// explicitly cast or forwarded back to an rvalue.
int&& bounce_bad(int&& value) {
  return value;
}

int main() {
  return 0;
}
