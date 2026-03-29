// Reduced repro for the record-member template prelude path in `types.cpp`:
// a member-template requires-clause must not treat `std::is_array_v<T>` as a
// declaration boundary just because `is_array_v` is an identifier after `::`.
// RUN: %c4cll --parse-only %s

namespace std {
template<typename T>
inline constexpr bool is_array_v = false;
}

template<typename T>
concept member_begin = true;

template<typename T>
concept adl_begin = true;

struct begin_box {
  template<typename T>
  requires std::is_array_v<T> || member_begin<T&> || adl_begin<T&>
  auto begin_like(T& t);
};
