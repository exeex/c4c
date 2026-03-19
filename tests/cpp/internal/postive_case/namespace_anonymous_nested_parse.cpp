// Parse test: anonymous namespaces nested inside a named namespace should not
// break later lookup from declarations in that enclosing namespace.
namespace outer {
namespace {
int hidden = 37;
}

int read_hidden() {
  return hidden;
}
}

int main() {
  return outer::read_hidden() - 37;
}
