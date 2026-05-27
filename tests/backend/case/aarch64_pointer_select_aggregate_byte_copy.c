typedef unsigned char u8;

struct Bytes4 {
  u8 x[4];
};

int copy_selected(int choose) {
  struct Bytes4 left = {"abcd"};
  struct Bytes4 right = {"1234"};
  struct Bytes4 *selected;
  if (choose) {
    selected = &left;
  } else {
    selected = &right;
  }
  struct Bytes4 copy = *selected;

  return copy.x[0] + copy.x[1] + copy.x[2] + copy.x[3];
}

int main(void) {
  return copy_selected(1) == 394 ? 0 : 1;
}
