// Parse-only regression: explicit operator-name member calls after `.` / `->`
// should stay in a single postfix expression chain.

struct Inner {
  int value;
};

struct Wrapper {
  Inner inner;

  Inner* operator->() {
    return &inner;
  }
};

int read(Wrapper& wrapper) {
  return wrapper.operator->()->value;
}

int main() {
  return 0;
}
