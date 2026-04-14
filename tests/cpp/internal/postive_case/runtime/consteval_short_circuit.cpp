// Test: consteval logical operators should short-circuit.

int runtime_value();

consteval int short_circuit_or() {
  return 1 || runtime_value();
}

consteval int short_circuit_and() {
  return 0 && runtime_value();
}

int main() {
  int a = short_circuit_or();
  if (a != 1) return 1;

  int b = short_circuit_and();
  if (b != 0) return 2;

  return 0;
}
