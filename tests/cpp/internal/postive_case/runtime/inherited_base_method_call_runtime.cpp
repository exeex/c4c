struct Base {
  bool is_zero() const { return true; }
};

struct Derived : Base {
};

int main() {
  Derived value;
  if (!value.is_zero())
    return 1;
  return 0;
}
