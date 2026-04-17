// Parse test: one namespace can reference declarations from another namespace
// with a qualified name.
namespace xx {
int some_xx_var = 47;
}

namespace oo {
int read_xx() {
  return xx::some_xx_var;
}
}

int main() {
  return oo::read_xx() - 47;
}
