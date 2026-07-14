#pragma once

// Shared LIR identity model. This header owns the single LirValueId
// definition so both operands.hpp and the public ir.hpp package index use the
// same type without a numeric mirror or a second identity namespace.

#include <cstdint>
#include <limits>

#include "../../shared/text_id_table.hpp"

namespace c4c::codegen::lir {

struct LirValueId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirValueId invalid() {
    return LirValueId{kInvalid};
  }
};

// Identity for the one selected current-function local object family.  This is
// deliberately distinct from SSA value identity: a pointer definition names
// an object, but does not turn every value into an object reference.
struct LirObjectId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirObjectId invalid() {
    return LirObjectId{kInvalid};
  }
};

[[nodiscard]] constexpr bool operator==(LirValueId lhs, LirValueId rhs) {
  return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(LirValueId lhs, LirValueId rhs) {
  return !(lhs == rhs);
}

[[nodiscard]] constexpr bool operator==(LirObjectId lhs, LirObjectId rhs) {
  return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(LirObjectId lhs, LirObjectId rhs) {
  return !(lhs == rhs);
}

}  // namespace c4c::codegen::lir
