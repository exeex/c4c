// Parse test: a declaration inside a namespace can be referenced through the
// global qualifier and its full namespace path.
namespace oo {
int a = 53;

int read_self() {
  return ::oo::a;
}
}

int main() {
  return oo::read_self() - 53;
}
