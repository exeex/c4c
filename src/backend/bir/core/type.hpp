#pragma once

#include <cstdint>

namespace c4c::backend::bir {

enum class TypeKind : std::uint8_t {
  Void,
  I1,
  I8,
  I16,
  I32,
  I64,
  F32,
  F64,
  Pointer,
};

struct Type {
  TypeKind kind = TypeKind::Void;
};

constexpr bool operator==(Type lhs, Type rhs) noexcept { return lhs.kind == rhs.kind; }
constexpr bool operator!=(Type lhs, Type rhs) noexcept { return !(lhs == rhs); }

}  // namespace c4c::backend::bir
