#pragma once

#include "../prealloc.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare::stack_layout {

[[nodiscard]] inline std::size_t fallback_type_size(c4c::backend::bir::TypeKind type) {
  using TypeKind = c4c::backend::bir::TypeKind;
  switch (type) {
    case TypeKind::Void:
      return 0;
    case TypeKind::I1:
    case TypeKind::I8:
      return 1;
    case TypeKind::I32:
    case TypeKind::F32:
      return 4;
    case TypeKind::I64:
    case TypeKind::Ptr:
    case TypeKind::F64:
      return 8;
    case TypeKind::I128:
    case TypeKind::F128:
      return 16;
  }
  return 0;
}

[[nodiscard]] inline std::size_t normalize_size(c4c::backend::bir::TypeKind type,
                                                std::size_t size_bytes) {
  return size_bytes == 0 ? fallback_type_size(type) : size_bytes;
}

[[nodiscard]] inline std::size_t normalize_alignment(c4c::backend::bir::TypeKind type,
                                                     std::size_t align_bytes,
                                                     std::size_t size_bytes) {
  const std::size_t normalized_size = normalize_size(type, size_bytes);
  if (align_bytes != 0) {
    return align_bytes;
  }
  if (normalized_size == 0) {
    return 1;
  }
  return std::min<std::size_t>(normalized_size, 16);
}

[[nodiscard]] inline std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment <= 1) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

}  // namespace c4c::backend::prepare::stack_layout
