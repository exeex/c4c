// Runtime test: operator[] member call on a struct.
struct Vec {
  int data[4];
  int sz;

  int operator[](int idx) {
    return data[idx];
  }
};

int main() {
  Vec v;
  v.data[0] = 10;
  v.data[1] = 20;
  v.data[2] = 30;
  v.data[3] = 40;
  v.sz = 4;

  if (v[0] != 10) return 1;
  if (v[1] != 20) return 2;
  if (v[2] != 30) return 3;
  if (v[3] != 40) return 4;
  return 0;
}
