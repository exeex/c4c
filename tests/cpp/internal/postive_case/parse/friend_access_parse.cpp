// Parse-only regression: friend declarations and inline friend definitions
// inside a class body should not terminate the class parse early.

struct FriendBox {
  int value;

  friend struct FriendAccessor;
  friend bool operator==(const FriendBox& a, const FriendBox& b) noexcept {
    return a.value == b.value;
  }

private:
  static int marker() { return 5; }
};

struct FriendAccessor {
  static int read(const FriendBox& box) { return box.value; }
};

int main() {
  FriendBox a{5};
  FriendBox b{5};
  return (a == b) ? (FriendAccessor::read(a) - FriendBox::marker()) : 1;
}
