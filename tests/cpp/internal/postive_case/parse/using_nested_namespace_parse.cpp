// Parse test: using-declaration should import a member from a multi-level
// namespace path.
namespace alpha {
namespace beta {
int value = 41;
}
}

using alpha::beta::value;

int main() {
  return value - 41;
}
