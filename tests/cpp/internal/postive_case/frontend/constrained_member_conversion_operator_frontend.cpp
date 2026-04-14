// Reduced frontend regression from libstdc++ ranges::subrange: a constrained
// member-template conversion operator inside a class template must not eject
// later record members into top-level declarations.

template<typename T>
concept C = true;

template<typename T>
concept Copyable = true;

template<typename It, typename Sent>
struct Box {
  It _M_begin;
  Sent _M_end;

  template<C Pair>
  constexpr operator Pair() const {
    return Pair(_M_begin, _M_end);
  }

  constexpr It begin() const requires Copyable<It> { return _M_begin; }

  Box& advance(int __n) {
    _M_begin = _M_begin;
    _M_end = _M_end;
    return *this;
  }
};

int main() {
  Box<int*, int*> b{};
  b.advance(1);
  return 0;
}
