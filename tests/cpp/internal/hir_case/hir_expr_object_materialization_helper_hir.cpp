// HIR regression: object-materialization helpers extracted from hir_expr.cpp
// must preserve direct constructor temps, initializer-list materialization,
// and class-object new/delete lowering.

typedef unsigned long size_t;

namespace std {

template <class T>
class initializer_list {
 public:
  constexpr initializer_list() : _M_array(nullptr), _M_len(0) {}

  constexpr const T* begin() const { return _M_array; }
  constexpr const T* end() const { return _M_array + _M_len; }
  constexpr unsigned long size() const { return _M_len; }

 private:
  const T* _M_array;
  unsigned long _M_len;
};

}  // namespace std

static unsigned char g_storage[64];

void* operator new(size_t) { return (void*)g_storage; }

void operator delete(void*) {}

struct Box {
  int value;

  Box(int&& v) : value(v) {}
};

int sum_list(std::initializer_list<int> values) { return (int)values.size(); }

int main() {
  Box local(3);
  int count = sum_list({1, 4, 7});
  Box* heap = new Box(5);
  delete heap;
  return local.value + count;
}
