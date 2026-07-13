#pragma once

#include <cassert>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::bir {

template <class T, class E>
class Result {
  static_assert(!std::is_same_v<T, E>, "Result value and error types must differ");
  static_assert(!std::is_reference_v<T> && !std::is_reference_v<E>,
                "Result does not store references");
  static_assert(!std::is_void_v<T> && !std::is_void_v<E>,
                "use Result<void, E> for a void success value");

 public:
  static Result success(T value) { return Result(ValueTag{}, std::move(value)); }
  static Result failure(E error) { return Result(ErrorTag{}, std::move(error)); }

  bool has_value() const noexcept { return storage_.index() == 0; }
  explicit operator bool() const noexcept { return has_value(); }

  T& value() & {
    assert(has_value());
    return std::get<0>(storage_);
  }
  const T& value() const& {
    assert(has_value());
    return std::get<0>(storage_);
  }
  T&& value() && {
    assert(has_value());
    return std::get<0>(std::move(storage_));
  }

  E& error() & {
    assert(!has_value());
    return std::get<1>(storage_);
  }
  const E& error() const& {
    assert(!has_value());
    return std::get<1>(storage_);
  }

 private:
  struct ValueTag {};
  struct ErrorTag {};

  Result(ValueTag, T value) : storage_(std::in_place_index<0>, std::move(value)) {}
  Result(ErrorTag, E error) : storage_(std::in_place_index<1>, std::move(error)) {}

  std::variant<T, E> storage_;
};

template <class E>
class Result<void, E> {
  static_assert(!std::is_reference_v<E> && !std::is_void_v<E>,
                "Result error must be a non-void value type");

 public:
  static Result success() { return Result(ValueTag{}); }
  static Result failure(E error) { return Result(ErrorTag{}, std::move(error)); }

  bool has_value() const noexcept { return storage_.index() == 0; }
  explicit operator bool() const noexcept { return has_value(); }

  E& error() & {
    assert(!has_value());
    return std::get<1>(storage_);
  }
  const E& error() const& {
    assert(!has_value());
    return std::get<1>(storage_);
  }

 private:
  struct ValueTag {};
  struct ErrorTag {};

  explicit Result(ValueTag) : storage_(std::in_place_index<0>) {}
  Result(ErrorTag, E error) : storage_(std::in_place_index<1>, std::move(error)) {}

  std::variant<std::monostate, E> storage_;
};

}  // namespace c4c::backend::bir
