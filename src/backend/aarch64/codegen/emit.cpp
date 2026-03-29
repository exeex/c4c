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

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
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
  if (block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
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

const c4c::backend::BackendFunction* find_function(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.signature.name == name) return &function;
  }
  return nullptr;
}

std::optional<std::int64_t> parse_single_block_return_imm(
    const c4c::backend::BackendFunction& function) {
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || !function.signature.params.empty() ||
      function.signature.is_vararg || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32" || !block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

struct MinimalDirectCallSlice {
  std::string callee_name;
  std::int64_t callee_imm = 0;
};

struct MinimalDirectCallAddImmSlice {
  std::string callee_name;
  std::int64_t arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct MinimalDirectCallTwoArgAddSlice {
  std::string callee_name;
  std::int64_t arg0_imm = 0;
  std::int64_t arg1_imm = 0;
};

std::optional<MinimalDirectCallSlice> parse_minimal_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      !call->callee_type_suffix.empty() || !call->args_str.empty() ||
      call->callee.empty() || call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr) return std::nullopt;
  const auto callee_imm = parse_single_block_return_imm(*callee_fn);
  if (!callee_imm.has_value()) return std::nullopt;

  return MinimalDirectCallSlice{callee_name, *callee_imm};
}

std::optional<MinimalDirectCallAddImmSlice> parse_minimal_direct_call_add_imm_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      call->callee_type_suffix != "(i32)" || call->callee.empty() ||
      call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const std::string_view args_str = call->args_str;
  if (args_str.size() < 5 || args_str.substr(0, 4) != "i32 ") {
    return std::nullopt;
  }
  const auto arg_imm = parse_i64(args_str.substr(4));
  if (!arg_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr || callee_fn->is_declaration ||
      callee_fn->signature.linkage != "define" ||
      callee_fn->signature.return_type != "i32" ||
      callee_fn->signature.params.size() != 1 ||
      callee_fn->signature.params.front().type_str != "i32" ||
      callee_fn->signature.params.front().name.empty() ||
      callee_fn->signature.is_vararg || callee_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& callee_block = callee_fn->blocks.front();
  if (callee_block.label != "entry" || callee_block.insts.size() != 1 ||
      !callee_block.terminator.value.has_value() ||
      callee_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&callee_block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *callee_block.terminator.value != add->result ||
      add->lhs != callee_fn->signature.params.front().name) {
    return std::nullopt;
  }

  const auto add_imm = parse_i64(add->rhs);
  if (!add_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalDirectCallAddImmSlice{callee_name, *arg_imm, *add_imm};
}

std::optional<MinimalDirectCallTwoArgAddSlice>
parse_minimal_direct_call_two_arg_add_slice(const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result ||
      call->callee_type_suffix != "(i32, i32)" || call->callee.empty() ||
      call->callee.front() != '@') {
    return std::nullopt;
  }

  const std::string callee_name = call->callee.substr(1);
  if (callee_name == "main") return std::nullopt;

  const std::string_view args_str = call->args_str;
  const size_t comma = args_str.find(", ");
  if (comma == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view arg0 = args_str.substr(0, comma);
  const std::string_view arg1 = args_str.substr(comma + 2);
  if (arg0.size() < 5 || arg1.size() < 5 || arg0.substr(0, 4) != "i32 " ||
      arg1.substr(0, 4) != "i32 ") {
    return std::nullopt;
  }
  const auto arg0_imm = parse_i64(arg0.substr(4));
  const auto arg1_imm = parse_i64(arg1.substr(4));
  if (!arg0_imm.has_value() || !arg1_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, callee_name);
  if (callee_fn == nullptr || callee_fn->is_declaration ||
      callee_fn->signature.linkage != "define" ||
      callee_fn->signature.return_type != "i32" ||
      callee_fn->signature.params.size() != 2 ||
      callee_fn->signature.params[0].type_str != "i32" ||
      callee_fn->signature.params[1].type_str != "i32" ||
      callee_fn->signature.params[0].name.empty() ||
      callee_fn->signature.params[1].name.empty() ||
      callee_fn->signature.is_vararg || callee_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& callee_block = callee_fn->blocks.front();
  if (callee_block.label != "entry" || callee_block.insts.size() != 1 ||
      !callee_block.terminator.value.has_value() ||
      callee_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&callee_block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      add->type_str != "i32" || *callee_block.terminator.value != add->result ||
      add->lhs != callee_fn->signature.params[0].name ||
      add->rhs != callee_fn->signature.params[1].name) {
    return std::nullopt;
  }

  return MinimalDirectCallTwoArgAddSlice{callee_name, *arg0_imm, *arg1_imm};
}

void emit_function_prelude(std::ostringstream& out,
                           const c4c::backend::BackendModule& module,
                           std::string_view symbol,
                           bool is_global) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (is_global) {
    out << ".globl " << symbol << "\n";
  }
  if (is_darwin) {
    out << ".p2align 2\n";
  } else {
    out << ".type " << symbol << ", %function\n";
  }
  out << symbol << ":\n";
}

std::string emit_minimal_return_imm_asm(const c4c::backend::BackendModule& module,
                                        std::int64_t imm) {
  if (imm < 0 || imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, symbol, true);
  out
      << "  mov w0, #" << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_asm(const c4c::backend::BackendModule& module,
                                         const MinimalDirectCallSlice& slice) {
  if (slice.callee_imm < 0 ||
      slice.callee_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("helper return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov w0, #" << slice.callee_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::BackendModule& module,
    const MinimalDirectCallAddImmSlice& slice) {
  if (slice.arg_imm < 0 ||
      slice.arg_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("direct-call argument immediates outside the minimal mov-supported range");
  }
  if (slice.add_imm < 0 || slice.add_imm > 4095) {
    fail_unsupported("helper add immediates outside the minimal add-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  add w0, w0, #" << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  mov w0, #" << slice.arg_imm << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_two_arg_add_asm(
    const c4c::backend::BackendModule& module,
    const MinimalDirectCallTwoArgAddSlice& slice) {
  if (slice.arg0_imm < 0 ||
      slice.arg0_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("first direct-call argument immediates outside the minimal mov-supported range");
  }
  if (slice.arg1_imm < 0 ||
      slice.arg1_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("second direct-call argument immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  add w0, w0, w1\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  mov w0, #" << slice.arg0_imm << "\n"
      << "  mov w1, #" << slice.arg1_imm << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  validate_module(module);
  try {
    const auto adapted = c4c::backend::adapt_minimal_module(module);
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto slice = parse_minimal_direct_call_slice(adapted); slice.has_value()) {
      return emit_minimal_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_add_imm_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_two_arg_add_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_two_arg_add_asm(adapted, *slice);
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
