// Parse test: names declared in an anonymous namespace should still be
// reachable unqualified within the same translation unit.
namespace {
int hidden = 31;
}

int main() {
  return hidden - 31;
}
