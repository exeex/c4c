struct Inner {
  int values[3];
};

struct Pair {
  struct Inner inner;
  int tail;
};

int main(void) {
  struct Pair pair = {{4, 7, 9}, 11};
  struct Pair other = {{1, 2, 3}, 5};
  struct Pair *ptr = &pair;
  int idx = 1;
  int total = 0;

  total += ptr->inner.values[idx];
  total += idx ? pair.inner.values[2] : other.inner.values[0];
  total += sizeof(pair.inner.values);
  total += sizeof(struct Pair);
  total += (int)(unsigned char)250;
  total += (ptr->tail = 13);
  total += ptr->tail;
  total += ((ptr->inner.values[0] = 6), ptr->inner.values[0]);
  total += (&ptr->inner.values[0])[2];

  return total == 335 ? 0 : 1;
}
