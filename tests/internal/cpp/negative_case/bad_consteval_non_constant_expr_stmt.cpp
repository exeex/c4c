// Negative test: a non-constant expression statement in a consteval body must be rejected.

int runtime_value();

consteval int fold_me() {
  runtime_value();
  return 1;
}

int main() {
  return fold_me();
}
