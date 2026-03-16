// Negative test: divide-by-zero inside a consteval function must fail constant evaluation.

consteval int explode() {
  return 1 / 0;
}

int main() {
  return explode();
}
