namespace outer {
namespace inner {
int value;
}
}

namespace bridge {
using namespace outer::inner;

int read() {
  return value;
}
}
