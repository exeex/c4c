// Runtime test: functions from a namespace imported via "using namespace"
// must be callable without qualification.
namespace math {
  int square(int x) { return x * x; }
  int twice(int x) { return x + x; }
}

using namespace math;

int main() {
  if (square(5) != 25) return 1;
  if (twice(7) != 14) return 2;
  // Qualified call should still work
  if (math::square(3) != 9) return 3;
  return 0;
}
