// Runtime test: global using-directives for std and a user namespace should
// make both namespaces participate in unqualified lookup.
namespace std {
struct Box {
  int value;
  int get() const { return value; }
};
}

namespace user_ns {
using namespace std;

int read_box(Box box) {
  return box.get() + 2;
}
}

using namespace std;
using namespace user_ns;

int main() {
  Box box;
  box.value = 5;
  return read_box(box) - 7;
}
