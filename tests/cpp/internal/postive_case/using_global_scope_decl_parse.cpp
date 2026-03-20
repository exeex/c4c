// Parse test: using-declaration should accept a global-qualified target.
int exported_value = 9;

namespace wrap {
using ::exported_value;
}

int main() {
  return wrap::exported_value - 9;
}
