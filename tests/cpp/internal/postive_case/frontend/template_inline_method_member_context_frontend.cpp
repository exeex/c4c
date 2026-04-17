// Reduced frontend regression from libstdc++ ranges_util.h: inline methods of
// class templates must keep enclosing NTTP names, implicit member lookup, and
// `this` available during semantic validation.

template<typename It, typename Sent, bool StoreSize>
struct Subrange {
  It _M_begin;
  Sent _M_end;

  struct Size {
    int _M_size;
  };

  Size _M_size = {};

  Subrange& advance(int __n) {
    if constexpr (StoreSize) {
      _M_size._M_size += -__n;
      return *this;
    }

    _M_begin = _M_begin + __n;
    return *this;
  }
};

int main() {
  Subrange<int*, int*, true> s{};
  s.advance(1);
  return 0;
}
