// Parse test: non-bool conversion operators should be accepted.
struct BufferCursor {
  explicit operator char() const {
    return 'x';
  }

  explicit operator unsigned long long() const {
    return 7ULL;
  }
};

int main() {
  return 0;
}
