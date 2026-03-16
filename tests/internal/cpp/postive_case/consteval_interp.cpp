// Test: consteval function body interpretation
// The compiler should evaluate consteval calls at compile time.

consteval int square(int x) {
  return x * x;
}

consteval int add(int a, int b) {
  return a + b;
}

consteval int chain(int x) {
  return add(square(x), 1);
}

consteval int with_local(int x) {
  const int doubled = x * 2;
  return doubled + 1;
}

consteval int with_if(int x) {
  if (x > 0)
    return x;
  else
    return -x;
}

int main() {
  // Basic consteval call
  int a = square(5);
  if (a != 25) return 1;

  // Chained consteval calls
  int b = chain(3);
  if (b != 10) return 2;

  // Consteval with local const binding
  int c = with_local(7);
  if (c != 15) return 3;

  // Consteval with if
  int d = with_if(-4);
  if (d != 4) return 4;

  int e = with_if(3);
  if (e != 3) return 5;

  return 0;
}
