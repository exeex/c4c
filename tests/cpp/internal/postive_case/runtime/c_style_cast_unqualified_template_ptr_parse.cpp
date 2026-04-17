template <class T>
struct box {};

int main() {
  void* raw = 0;
  box<int>* p = (box<int>*)raw;
  return p == 0 ? 0 : 1;
}
