// Parse test: nested namespace definitions and multi-hop qualified lookup.
namespace outer {
namespace inner {
int value = 11;
}
}

int main() {
  return outer::inner::value - 11;
}
