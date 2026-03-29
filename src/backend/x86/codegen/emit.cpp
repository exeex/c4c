#include "emit.hpp"

#include "../../lir_adapter.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string_view>

// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs
// Stage the x86 backend behind an explicit seam while the real codegen path is
// still being brought up.

namespace c4c::backend::x86 {

namespace {

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

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

bool is_minimal_single_function_asm_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) return false;

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32") {
    return false;
  }

  return true;
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_return_add_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(add->lhs);
  const auto rhs = parse_i64(add->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs + *rhs;
}

std::string emit_minimal_return_asm(const c4c::backend::BackendModule& module,
                                    std::int64_t return_imm) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  try {
    const auto adapted = c4c::backend::adapt_minimal_module(module);
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  return c4c::codegen::lir::print_llvm(module);
}

}  // namespace c4c::backend::x86
