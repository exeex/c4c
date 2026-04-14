// Parse-only regression: friend operator<=> definitions inside a class body
// must not break parsing of the following members.
// RUN: %c4cll --parse-only %s

struct ordering {
  static ordering less;
  static ordering greater;
};

struct Box {
  friend ordering operator<=>(const Box& lhs, const Box& rhs) noexcept {
    return lhs.value < rhs.value ? ordering::less : ordering::greater;
  }

  int value;
  int marker;
};

int main() {
  return 0;
}
