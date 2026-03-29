#include "emit.hpp"

#include "../../lir_adapter.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace c4c::backend::aarch64 {

namespace {

[[noreturn]] void fail_unsupported(const char* detail) {
  throw std::invalid_argument(std::string("aarch64 backend emitter does not support ") +
                              detail);
}

void validate_inst(const c4c::codegen::lir::LirInst& inst) {
  if (std::holds_alternative<c4c::codegen::lir::LirSelectOp>(inst)) {
    fail_unsupported("non-ALU/non-branch/non-call/non-memory instructions");
  }
}

void validate_block(const c4c::codegen::lir::LirBlock& block) {
  for (const auto& inst : block.insts) {
    validate_inst(inst);
  }
}

void validate_function(const c4c::codegen::lir::LirFunction& function) {
  for (const auto& inst : function.alloca_insts) {
    validate_inst(inst);
  }
  for (const auto& block : function.blocks) {
    validate_block(block);
  }
}

void validate_module(const c4c::codegen::lir::LirModule& module) {
  for (const auto& function : module.functions) {
    validate_function(function);
  }
}

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool is_minimal_return_add_asm_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) return false;

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return false;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *block.terminator.value != add->result) {
    return false;
  }

  return parse_i64(add->lhs).has_value() && parse_i64(add->rhs).has_value();
}

std::string emit_minimal_return_add_asm(const c4c::backend::BackendModule& module) {
  const auto& add = std::get<c4c::backend::BackendBinaryInst>(
      module.functions.front().blocks.front().insts.front());
  const auto lhs = *parse_i64(add.lhs);
  const auto rhs = *parse_i64(add.rhs);
  const auto sum = lhs + rhs;
  if (sum < 0 || sum > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return-add immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string symbol = is_darwin ? "_main" : "main";

  out << ".text\n"
      << ".globl " << symbol << "\n";
  if (is_darwin) {
    out << ".p2align 2\n";
  } else {
    out << ".type " << symbol << ", %function\n";
  }
  out << symbol << ":\n"
      << "  mov w0, #" << sum << "\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  validate_module(module);
  try {
    const auto adapted = c4c::backend::adapt_minimal_module(module);
    if (is_minimal_return_add_asm_slice(adapted)) {
      return emit_minimal_return_add_asm(adapted);
    }
    return c4c::backend::render_module(adapted);
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return c4c::codegen::lir::print_llvm(module);
}

}  // namespace c4c::backend::aarch64
