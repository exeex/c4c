typedef struct {
  int head;
  union {
    int left;
    int right;
  };
  struct {
    union {
      struct {
        int inner;
      };
    };
  };
  int tail;
} Aggregate;

int main(void) {
  Aggregate value;

  value.head = 1;
  value.left = 2;
  value.inner = 3;
  value.tail = 4;

  if (value.head != 1)
    return 10;
  if (value.left != 2 || value.right != 2)
    return 20;
  if (value.inner != 3)
    return 30;
  if (value.tail != 4)
    return 40;

  return 0;
}
