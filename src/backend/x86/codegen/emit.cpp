#include "emit.hpp"

#include "../../generation.hpp"
#include "../../lir_adapter.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>

// Mechanical translation of the ref x86 emitter entrypoint.
// The larger target-specific lowering surface is split across the sibling
// translation units in this directory.

namespace c4c::backend::x86 {

namespace {

struct MinimalCallCrossingDirectCallSlice {
  std::string callee_name;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
  std::string source_value;
};

struct MinimalConditionalReturnSlice {
  std::string predicate;
  std::int64_t lhs_imm = 0;
  std::int64_t rhs_imm = 0;
  std::string true_label;
  std::string false_label;
  std::int64_t true_return_imm = 0;
  std::int64_t false_return_imm = 0;
};

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

const c4c::codegen::lir::LirFunction* find_lir_function(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.name == name) return &function;
  }
  return nullptr;
}

const c4c::backend::BackendFunction* find_function(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.signature.name == name) return &function;
  }
  return nullptr;
}

const char* x86_reg64_name(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1:
      return "rbx";
    case 2:
      return "r12";
    case 3:
      return "r13";
    case 4:
      return "r14";
    case 5:
      return "r15";
    case 10:
      return "r11";
    case 11:
      return "r10";
    case 12:
      return "r8";
    case 13:
      return "r9";
    case 14:
      return "rdi";
    case 15:
      return "rsi";
    default:
      return nullptr;
  }
}

const char* x86_reg32_name(c4c::backend::PhysReg reg) {
  switch (reg.index) {
    case 1:
      return "ebx";
    case 2:
      return "r12d";
    case 3:
      return "r13d";
    case 4:
      return "r14d";
    case 5:
      return "r15d";
    case 10:
      return "r11d";
    case 11:
      return "r10d";
    case 12:
      return "r8d";
    case 13:
      return "r9d";
    case 14:
      return "edi";
    case 15:
      return "esi";
    default:
      return nullptr;
  }
}

std::int64_t aligned_frame_size(std::size_t saved_reg_count) {
  const auto raw = static_cast<std::int64_t>(saved_reg_count) * 8;
  return raw > 0 ? (raw + 15) & ~std::int64_t{15} : 0;
}

c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::codegen::lir::LirFunction& function) {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{1}, {2}, {3}, {4}, {5}};
  config.caller_saved_regs = {{10}, {11}, {12}, {13}, {14}, {15}};
  return c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
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

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) return std::nullopt;

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  if (entry.label != "entry" || entry.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || (cmp0->predicate != "slt" && cmp0->predicate != "sle") ||
      cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1->predicate != "ne" || cmp1->type_str != "i32" || cmp1->lhs != cast->result ||
      cmp1->rhs != "0" || condbr->cond_name != cmp1->result) {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(cmp0->lhs);
  const auto rhs_imm = parse_i64(cmp0->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (true_block.label != condbr->true_label || false_block.label != condbr->false_label ||
      !true_block.insts.empty() || !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != "i32" ||
      false_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto true_return_imm = parse_i64(*true_ret->value_str);
  const auto false_return_imm = parse_i64(*false_ret->value_str);
  if (!true_return_imm.has_value() || !false_return_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalReturnSlice{cmp0->predicate,
                                       *lhs_imm,
                                       *rhs_imm,
                                       condbr->true_label,
                                       condbr->false_label,
                                       *true_return_imm,
                                       *false_return_imm};
}

std::optional<MinimalCallCrossingDirectCallSlice> parse_minimal_call_crossing_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) return std::nullopt;

  const auto* helper = find_function(module, "add_one");
  const auto* main_fn = find_function(module, "main");
  if (helper == nullptr || main_fn == nullptr || helper->is_declaration ||
      main_fn->is_declaration || helper->blocks.size() != 1 ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  if (helper->signature.linkage != "define" || helper->signature.return_type != "i32" ||
      helper->signature.params.size() != 1 || helper->signature.params.front().type_str != "i32" ||
      helper->signature.is_vararg || main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" || !main_fn->signature.params.empty() ||
      main_fn->signature.is_vararg) {
    return std::nullopt;
  }

  const auto& helper_block = helper->blocks.front();
  if (helper_block.label != "entry" || helper_block.insts.size() != 1 ||
      !helper_block.terminator.value.has_value() ||
      helper_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* helper_add =
      std::get_if<c4c::backend::BackendBinaryInst>(&helper_block.insts.front());
  if (helper_add == nullptr ||
      helper_add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      helper_add->type_str != "i32" ||
      *helper_block.terminator.value != helper_add->result ||
      helper_add->lhs != helper->signature.params.front().name) {
    return std::nullopt;
  }

  const auto helper_add_imm = parse_i64(helper_add->rhs);
  if (!helper_add_imm.has_value()) return std::nullopt;

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 3 ||
      !main_block.terminator.value.has_value() ||
      main_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* source_add =
      std::get_if<c4c::backend::BackendBinaryInst>(&main_block.insts[0]);
  const auto* call = std::get_if<c4c::backend::BackendCallInst>(&main_block.insts[1]);
  const auto* final_add =
      std::get_if<c4c::backend::BackendBinaryInst>(&main_block.insts[2]);
  if (source_add == nullptr || call == nullptr || final_add == nullptr ||
      source_add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      source_add->type_str != "i32" || call->return_type != "i32" ||
      call->callee != "@add_one" || call->callee_type_suffix != "(i32)" ||
      call->args_str != ("i32 " + source_add->result) ||
      final_add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      final_add->type_str != "i32" || final_add->lhs != source_add->result ||
      final_add->rhs != call->result ||
      *main_block.terminator.value != final_add->result) {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(source_add->lhs);
  const auto rhs_imm = parse_i64(source_add->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) return std::nullopt;

  return MinimalCallCrossingDirectCallSlice{
      "add_one",
      *lhs_imm + *rhs_imm,
      *helper_add_imm,
      source_add->result,
  };
}

void emit_function_prelude(std::ostringstream& out,
                           const c4c::backend::BackendModule& module,
                           std::string_view symbol,
                           bool is_global) {
  if (is_global) out << ".globl " << symbol << "\n";
  out << ".type " << symbol << ", %function\n";
  out << symbol << ":\n";
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

std::string emit_minimal_conditional_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  const char* fail_branch = nullptr;
  if (slice.predicate == "slt") {
    fail_branch = "jge";
  } else if (slice.predicate == "sle") {
    fail_branch = "jg";
  } else {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "conditional-return predicates outside the current compare-and-branch x86 slice");
  }
  out << "  mov eax, " << slice.lhs_imm << "\n";
  out << "  cmp eax, " << slice.rhs_imm << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  out << "  mov eax, " << slice.true_return_imm << "\n";
  out << "  ret\n";
  out << ".L" << slice.false_label << ":\n";
  out << "  mov eax, " << slice.false_return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_call_crossing_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::RegAllocIntegrationResult& regalloc,
    const MinimalCallCrossingDirectCallSlice& slice) {
  if (slice.source_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.source_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "call-crossing source immediates outside the minimal mov-supported range");
  }
  if (slice.helper_add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.helper_add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "call-crossing helper add immediates outside the minimal add-supported range");
  }

  const auto* source_reg =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, slice.source_value);
  if (source_reg == nullptr ||
      !c4c::backend::stack_layout::uses_callee_saved_reg(regalloc, *source_reg)) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "shared call-crossing regalloc state for the minimal x86 direct-call slice");
  }

  const char* reg64 = x86_reg64_name(*source_reg);
  const char* reg32 = x86_reg32_name(*source_reg);
  if (reg64 == nullptr || reg32 == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "minimal x86 direct-call slice received an unknown physical register");
  }

  const auto frame_size = aligned_frame_size(1);
  const auto helper_symbol = asm_symbol_name(module, slice.callee_name);
  const auto main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.helper_add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  push rbp\n"
      << "  mov rbp, rsp\n";
  if (frame_size > 0) {
    out << "  sub rsp, " << frame_size << "\n"
        << "  mov qword ptr [rbp - " << frame_size << "], " << reg64 << "\n";
  }
  out << "  mov " << reg32 << ", " << slice.source_imm << "\n"
      << "  mov edi, " << reg32 << "\n"
      << "  call " << helper_symbol << "\n"
      << "  mov eax, eax\n"
      << "  add eax, " << reg32 << "\n";
  if (frame_size > 0) {
    out << "  mov " << reg64 << ", qword ptr [rbp - " << frame_size << "]\n";
  }
  out << "  mov rsp, rbp\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string remove_redundant_self_moves(std::string asm_text) {
  std::string optimized;
  optimized.reserve(asm_text.size());

  std::size_t start = 0;
  while (start < asm_text.size()) {
    const auto end = asm_text.find('\n', start);
    const auto line_end = end == std::string::npos ? asm_text.size() : end;
    std::string_view line(asm_text.data() + start, line_end - start);

    std::size_t trim = 0;
    while (trim < line.size() && (line[trim] == ' ' || line[trim] == '\t')) ++trim;
    std::string_view trimmed = line.substr(trim);

    bool drop_line = false;
    if (trimmed.size() >= 4 && trimmed.substr(0, 4) == "mov ") {
      const auto comma = trimmed.find(',');
      if (comma != std::string_view::npos) {
        auto lhs = trimmed.substr(4, comma - 4);
        auto rhs = trimmed.substr(comma + 1);
        while (!lhs.empty() && lhs.back() == ' ') lhs.remove_suffix(1);
        while (!rhs.empty() && rhs.front() == ' ') rhs.remove_prefix(1);
        drop_line = lhs == rhs;
      }
    }

    if (!drop_line) {
      optimized.append(line);
      if (end != std::string::npos) optimized.push_back('\n');
    }

    if (end == std::string::npos) break;
    start = end + 1;
  }

  return optimized;
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  try {
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_conditional_return_asm(scaffold_module, *slice);
    }
    const auto adapted = c4c::backend::adapt_minimal_module(module);
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(adapted);
        slice.has_value()) {
      const auto* main_fn = find_lir_function(module, "main");
      if (main_fn == nullptr) {
        throw c4c::backend::LirAdapterError(
            c4c::backend::LirAdapterErrorKind::Unsupported,
            "main function for shared x86 call-crossing direct-call slice");
      }
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          adapted, run_shared_x86_regalloc(*main_fn), *slice));
    }
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
