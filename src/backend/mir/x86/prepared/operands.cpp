#include "prepared.hpp"

namespace c4c::backend::x86::prepared {

std::optional<Operand> render_immediate_operand(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  // Immediate spelling is final operand text derived from the structured BIR
  // value; it must not be reused as prepared value identity.
  return Operand{
      .text = std::to_string(value.immediate),
      .materialize = false,
  };
}

}  // namespace c4c::backend::x86::prepared
