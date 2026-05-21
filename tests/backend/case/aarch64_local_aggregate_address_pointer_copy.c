typedef unsigned char u8;

struct ByteBox {
  u8 a;
  u8 b;
  u8 c[2];
};

struct Tail {
  u8 bytes[16];
  u8 marker;
};

struct Wrapped {
  u8 head;
  struct ByteBox box;
  u8 middle;
  struct Tail tail;
};

int main(void) {
  struct ByteBox local = {1, 2, {3, 4}};
  struct Wrapped first = {3, local, 4, {"huhu", 43}};
  struct Wrapped second = {3, (local), 4, {"huhu", 43}};
  const struct ByteBox *pointer = &local;
  struct ByteBox copy = *pointer;
  return copy.a + copy.b + copy.c[0] + copy.c[1] + first.head + second.middle;
}
