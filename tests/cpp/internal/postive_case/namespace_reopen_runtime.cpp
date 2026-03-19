// Runtime test: reopening a namespace should merge into the same logical context.
namespace math {
int seed = 9;
}

namespace math {
int read_seed() {
  return seed;
}
}

int main() {
  return math::read_seed() - 9;
}
