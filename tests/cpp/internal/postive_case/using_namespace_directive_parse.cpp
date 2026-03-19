// Parse test: using-directive should make namespace members visible.
namespace store {
int value = 17;
}

using namespace store;

int main() {
  return value - 17;
}
