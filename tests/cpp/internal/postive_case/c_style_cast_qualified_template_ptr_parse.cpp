namespace std {
template <class T>
struct vector {
  T* data;
};
}

int main() {
  void* raw = 0;
  std::vector<int>* p = (std::vector<int>*)raw;
  return p == 0 ? 0 : 1;
}
