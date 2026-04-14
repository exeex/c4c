// Parse-only regression: access labels should be tolerated in class bodies
// and not interfere with the following member declarations.

struct AccessBox {
public:
  int x;

protected:
  int y() { return 2; }

private:
  int z() { return 3; }
};

int main() {
  AccessBox box;
  box.x = 1;
  return box.x - 1;
}
