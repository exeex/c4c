// Parse-only regression: C++11 [[...]] attributes in front of a templated
// declaration must not throw the parser out of sync for following declarations.

template<typename T>
struct byte_operand {
};

template<typename T>
using byte_op_t = byte_operand<T>;

template<typename IntegerType>
[[nodiscard, __gnu__::__always_inline__]]
constexpr IntegerType to_integer(byte_op_t<IntegerType> value) noexcept {
  return IntegerType(value);
}

struct EANonCopyable {
  EANonCopyable() = default;
  ~EANonCopyable() = default;
  EANonCopyable(const EANonCopyable&) = delete;
  void operator=(const EANonCopyable&) = delete;
};

int main() { return 0; }
