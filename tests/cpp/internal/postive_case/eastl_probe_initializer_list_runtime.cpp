// Probe: initializer_list runtime behavior should only succeed when brace-init,
// size, and iteration all behave as expected.
//
// Keep this test self-contained so it does not depend on libc++ header parsing
// details that vary across host platforms.

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

int sum_and_check(std::initializer_list<int> values) {
    if (values.size() != 3)
        return 1;
    if (*values.begin() != 1)
        return 2;

    int sum = 0;
    for (const int* it = values.begin(); it != values.end(); ++it)
        sum += *it;

    return sum == 12 ? 0 : 3;
}

int main() {
    return sum_and_check({1, 4, 7});
}
