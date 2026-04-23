#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/prealloc.hpp"

#include <string>
#include <string_view>

namespace c4c::backend::x86::module {

struct Data {
  const c4c::backend::prepare::PreparedBirModule* module = nullptr;
  std::string_view target_triple;

  [[nodiscard]] std::string render_asm_symbol_name(std::string_view logical_name) const;
  [[nodiscard]] std::string render_private_data_label(std::string_view pool_name) const;
  [[nodiscard]] std::string emit_data() const;
};

[[nodiscard]] Data make_data(const c4c::backend::prepare::PreparedBirModule& module,
                             std::string_view target_triple);
[[nodiscard]] std::string emit(const c4c::backend::prepare::PreparedBirModule& module);

// Module emission is allowed to:
// - spell GPR/FPR/VREG registers and stack operands from prepared plans
// - emit prologue/epilogue and call text from prepared plans
// - emit published dynamic-stack operations for VLA / dynamic alloca regions
//
// Module emission is not allowed to:
// - choose caller-vs-callee saved policy
// - redo register allocation
// - invent stack-slot placement, fixed-slot anchoring, or call ABI routing

}  // namespace c4c::backend::x86::module
