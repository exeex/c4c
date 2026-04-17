// Parse test: basic namespace definition and qualified variable use.
namespace math {
int value = 7;
}

int main() {
  return math::value - 7;
}
