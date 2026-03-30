// Parse-only regression: friend operator< members must not be mistaken for
// template-angle depth and swallow the following record members.
// RUN: %c4cll --parse-only %s

struct Box {
  int value;

  friend bool operator<(const Box& lhs, const Box& rhs) noexcept {
    return lhs.value < rhs.value;
  }

  friend bool operator<=(const Box& lhs, const Box& rhs) noexcept {
    return !(rhs < lhs);
  }

  int marker;
};

int main() {
  return 0;
}
