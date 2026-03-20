// Runtime test: nested namespaces should resolve names imported by an inner
// using-directive alongside outer namespace members.
namespace std {
int offset() {
  return 3;
}
}

namespace outer {
int base() {
  return 8;
}

namespace inner {
using namespace std;

int total() {
  return offset() + outer::base();
}
}
}

int main() {
  return outer::inner::total() - 11;
}
