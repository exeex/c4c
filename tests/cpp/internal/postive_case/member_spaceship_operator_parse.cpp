// Parse-only regression: record methods named operator<=> should parse as
// ordinary method members and preserve following fields.
// RUN: %c4cll --parse-only %s

struct ordering {
  static ordering less;
  static ordering greater;
};

struct Box {
  ordering operator<=>(const Box& rhs) const noexcept {
    return value < rhs.value ? ordering::less : ordering::greater;
  }

  int value;
  int marker;
};

int main() {
  return 0;
}
