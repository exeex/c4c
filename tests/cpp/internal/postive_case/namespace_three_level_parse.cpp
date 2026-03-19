// Parse test: three-level namespace nesting should preserve the full
// qualification chain for declarations and references.
namespace alpha {
namespace beta {
namespace gamma {
int value = 23;
}
}
}

int main() {
  return alpha::beta::gamma::value - 23;
}
