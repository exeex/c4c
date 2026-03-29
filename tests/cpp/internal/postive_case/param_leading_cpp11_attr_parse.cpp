template<class U>
struct allocator {};

template<class T, class U>
void relocate(T* first, T* last, T* result,
              [[__maybe_unused__]] allocator<U>& alloc) {}

int main() {
  return 0;
}
