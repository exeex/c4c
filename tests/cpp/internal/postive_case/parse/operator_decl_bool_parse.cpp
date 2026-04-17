// Parse test: operator bool conversion.
struct Handle {
  int fd;

  operator bool() {
    return fd >= 0;
  }
};

int main() {
  return 0;
}
