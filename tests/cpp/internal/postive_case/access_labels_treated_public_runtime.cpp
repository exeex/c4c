// Parse-only coverage: access labels are currently tolerated syntactically.
// We cannot make this a runtime reference test because host clang enforces
// real C++ access control before c4cll runs.

struct AccessRuntime {
public:
  int get_public() { return 1; }

protected:
  int get_protected() { return 2; }

private:
  int get_private() { return 3; }
};

int main() {
  AccessRuntime box;
  return box.get_public() - 1;
}
