// Runtime test: a reopened namespace may introduce a using-directive and use
// imported std members together with its own declarations.
namespace std {
int helper() {
  return 6;
}
}

namespace xx {
int local() {
  return 4;
}
}

namespace xx {
using namespace std;

int total() {
  return helper() + local();
}
}

int main() {
  return xx::total() - 10;
}
