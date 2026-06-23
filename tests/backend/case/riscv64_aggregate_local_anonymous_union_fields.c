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

  return (value.head - 1) + (value.left - 2) + (value.right - 2) +
         (value.inner - 3) + (value.tail - 4);
}
