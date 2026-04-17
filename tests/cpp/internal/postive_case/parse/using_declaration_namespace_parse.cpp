// Parse test: using-declaration should import a namespace member.
namespace store {
int value = 13;
}

using store::value;

int main() {
  return value - 13;
}
