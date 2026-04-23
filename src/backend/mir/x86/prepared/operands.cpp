#include "prepared.hpp"

namespace c4c::backend::x86::prepared {

std::optional<Operand> render_immediate_operand(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  return Operand{
      .text = std::to_string(value.immediate),
      .materialize = false,
  };
}

}  // namespace c4c::backend::x86::prepared
