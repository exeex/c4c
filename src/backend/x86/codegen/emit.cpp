#include "emit.hpp"

#include "../../generation.hpp"
#include "../../ir_printer.hpp"
#include "../../lir_adapter.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>
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

struct MinimalDirectCallSlice {
  std::string callee_name;
  std::int64_t callee_imm = 0;
};

struct MinimalDirectCallAddImmSlice {
  std::string callee_name;
  std::int64_t arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct MinimalParamSlotSlice {
  std::string callee_name;
  std::int64_t call_arg_imm = 0;
  std::int64_t helper_add_imm = 0;
};

struct MinimalTwoArgDirectCallSlice {
  std::string callee_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalConditionalReturnSlice {
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  std::int64_t lhs_imm = 0;
  std::int64_t rhs_imm = 0;
  std::string true_label;
  std::string false_label;
  std::int64_t true_return_imm = 0;
  std::int64_t false_return_imm = 0;
};

struct MinimalConditionalPhiJoinSlice {
  struct IncomingValue {
    std::int64_t base_imm = 0;
    std::optional<std::int64_t> add_imm;
  };

  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  std::int64_t lhs_imm = 0;
  std::int64_t rhs_imm = 0;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  IncomingValue true_value;
  IncomingValue false_value;
  std::optional<std::int64_t> join_add_imm;
};

struct MinimalCountdownLoopSlice {
  std::int64_t initial_imm = 0;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
};

struct MinimalLocalArraySlice {
  std::int64_t store0_imm = 0;
  std::int64_t store1_imm = 0;
};

struct MinimalExternGlobalArrayLoadSlice {
  std::string global_name;
  std::int64_t byte_offset = 0;
};

struct MinimalExternDeclCallSlice {
  std::string callee_name;
};

struct MinimalGlobalCharPointerDiffSlice {
  std::string global_name;
  std::int64_t global_size = 0;
  std::int64_t byte_offset = 0;
};

struct MinimalGlobalIntPointerDiffSlice {
  std::string global_name;
  std::int64_t global_size = 0;
  std::int64_t byte_offset = 0;
  std::int64_t element_shift = 0;
};

struct MinimalScalarGlobalLoadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
};

struct MinimalStringLiteralCharSlice {
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_offset = 0;
  bool sign_extend = true;
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

std::optional<std::string_view> strip_typed_operand_prefix(std::string_view operand,
                                                           std::string_view type_prefix) {
  if (operand.size() <= type_prefix.size() + 1 ||
      operand.substr(0, type_prefix.size()) != type_prefix ||
      operand[type_prefix.size()] != ' ') {
    return std::nullopt;
  }
  return operand.substr(type_prefix.size() + 1);
}

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name) {
  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

std::string asm_private_data_label(const c4c::codegen::lir::LirModule& module,
                                   std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  const bool is_darwin =
      module.target_triple.find("apple-darwin") != std::string::npos;
  if (is_darwin) {
    return "L." + label;
  }
  return ".L." + label;
}

std::string escape_asm_string(std::string_view bytes) {
  static constexpr char kHexDigits[] = "0123456789abcdef";

  std::string escaped;
  escaped.reserve(bytes.size());
  for (unsigned char ch : bytes) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (ch >= 0x20 && ch <= 0x7e) {
          escaped.push_back(static_cast<char>(ch));
        } else {
          escaped += "\\x";
          escaped.push_back(kHexDigits[(ch >> 4) & 0xf]);
          escaped.push_back(kHexDigits[ch & 0xf]);
        }
        break;
    }
  }
  return escaped;
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

const c4c::backend::BackendGlobal* find_global(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& global : module.globals) {
    if (global.name == name) return &global;
  }
  return nullptr;
}

const c4c::backend::BackendStringConstant* find_string_constant(
    const c4c::backend::BackendModule& module,
    std::string_view name) {
  for (const auto& string_constant : module.string_constants) {
    if (string_constant.name == name) return &string_constant;
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

std::optional<std::int64_t> parse_minimal_return_sub_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* sub = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::BackendBinaryOpcode::Sub ||
      sub->type_str != "i32" || *block.terminator.value != sub->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(sub->lhs);
  const auto rhs = parse_i64(sub->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs - *rhs;
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 3) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (entry.label != "entry" || entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      true_block.label != entry.terminator.true_label ||
      false_block.label != entry.terminator.false_label ||
      !true_block.insts.empty() || !false_block.insts.empty() ||
      true_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      false_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !true_block.terminator.value.has_value() || !false_block.terminator.value.has_value() ||
      true_block.terminator.type_str != "i32" || false_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  if (cmp == nullptr || cmp->result != entry.terminator.cond_name || cmp->type_str != "i32") {
    return std::nullopt;
  }

  const auto lhs_imm = parse_i64(cmp->lhs);
  const auto rhs_imm = parse_i64(cmp->rhs);
  const auto true_return_imm = parse_i64(*true_block.terminator.value);
  const auto false_return_imm = parse_i64(*false_block.terminator.value);
  if (!lhs_imm.has_value() || !rhs_imm.has_value() || !true_return_imm.has_value() ||
      !false_return_imm.has_value()) {
    return std::nullopt;
  }

  using BackendPred = c4c::backend::BackendComparePredicate;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;
  LirPred predicate = LirPred::Eq;
  switch (cmp->predicate) {
    case BackendPred::Slt: predicate = LirPred::Slt; break;
    case BackendPred::Sle: predicate = LirPred::Sle; break;
    case BackendPred::Sgt: predicate = LirPred::Sgt; break;
    case BackendPred::Sge: predicate = LirPred::Sge; break;
    case BackendPred::Eq: predicate = LirPred::Eq; break;
    case BackendPred::Ne: predicate = LirPred::Ne; break;
    case BackendPred::Ult: predicate = LirPred::Ult; break;
    case BackendPred::Ule: predicate = LirPred::Ule; break;
    case BackendPred::Ugt: predicate = LirPred::Ugt; break;
    case BackendPred::Uge: predicate = LirPred::Uge; break;
  }

  return MinimalConditionalReturnSlice{predicate,
                                       *lhs_imm,
                                       *rhs_imm,
                                       true_block.label,
                                       false_block.label,
                                       *true_return_imm,
                                       *false_return_imm};
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
  const auto cmp0_predicate = cmp0 == nullptr ? std::nullopt : cmp0->predicate.typed();
  const auto cmp1_predicate = cmp1 == nullptr ? std::nullopt : cmp1->predicate.typed();
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || !cmp0_predicate.has_value() ||
      (*cmp0_predicate != LirCmpPredicate::Slt && *cmp0_predicate != LirCmpPredicate::Sle &&
       *cmp0_predicate != LirCmpPredicate::Sgt && *cmp0_predicate != LirCmpPredicate::Sge &&
       *cmp0_predicate != LirCmpPredicate::Eq && *cmp0_predicate != LirCmpPredicate::Ne &&
       *cmp0_predicate != LirCmpPredicate::Ult && *cmp0_predicate != LirCmpPredicate::Ule &&
       *cmp0_predicate != LirCmpPredicate::Ugt && *cmp0_predicate != LirCmpPredicate::Uge) ||
      cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" ||
      cmp1->lhs != cast->result || cmp1->rhs != "0" || condbr->cond_name != cmp1->result) {
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

  return MinimalConditionalReturnSlice{*cmp0_predicate,
                                       *lhs_imm,
                                       *rhs_imm,
                                       condbr->true_label,
                                       condbr->false_label,
                                       *true_return_imm,
                                       *false_return_imm};
}

std::optional<MinimalCountdownLoopSlice> parse_minimal_countdown_loop_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& loop = function.blocks[1];
  const auto& body = function.blocks[2];
  const auto& exit = function.blocks[3];
  if (entry.label != "entry" || !entry.insts.empty() ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      entry.terminator.target_label != loop.label || loop.insts.size() != 2 ||
      loop.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      loop.terminator.true_label != body.label ||
      loop.terminator.false_label != exit.label || body.insts.size() != 1 ||
      body.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      body.terminator.target_label != loop.label || !exit.insts.empty() ||
      exit.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !exit.terminator.value.has_value() || exit.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&loop.insts[0]);
  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&loop.insts[1]);
  const auto* dec = std::get_if<c4c::backend::BackendBinaryInst>(&body.insts.front());
  if (phi == nullptr || cmp == nullptr || dec == nullptr || phi->type_str != "i32" ||
      phi->incoming.size() != 2 || phi->incoming[0].label != entry.label ||
      phi->incoming[1].label != body.label || cmp->predicate != c4c::backend::BackendComparePredicate::Ne ||
      cmp->result != loop.terminator.cond_name || cmp->type_str != "i32" ||
      cmp->lhs != phi->result || cmp->rhs != "0" ||
      dec->opcode != c4c::backend::BackendBinaryOpcode::Sub || dec->type_str != "i32" ||
      dec->lhs != phi->result || dec->rhs != "1" || dec->result != phi->incoming[1].value ||
      *exit.terminator.value != phi->result) {
    return std::nullopt;
  }

  const auto initial_imm = parse_i64(phi->incoming[0].value);
  if (!initial_imm.has_value() || *initial_imm < 0) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{*initial_imm, loop.label, body.label, exit.label};
}

std::optional<MinimalConditionalPhiJoinSlice> parse_minimal_conditional_phi_join_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature.linkage != "define" ||
      function.signature.return_type != "i32" || function.signature.name != "main" ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  const auto& join_block = function.blocks[3];
  const auto parse_incoming_value =
      [](const c4c::backend::BackendBlock& block,
         std::string_view expected_result)
      -> std::optional<MinimalConditionalPhiJoinSlice::IncomingValue> {
    if (block.insts.empty()) {
      const auto imm = parse_i64(expected_result);
      if (!imm.has_value()) {
        return std::nullopt;
      }
      return MinimalConditionalPhiJoinSlice::IncomingValue{*imm, std::nullopt};
    }
    if (block.insts.size() != 1) {
      return std::nullopt;
    }

    const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
    if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
        add->type_str != "i32" || add->result != expected_result) {
      return std::nullopt;
    }

    const auto base_imm = parse_i64(add->lhs);
    const auto add_imm = parse_i64(add->rhs);
    if (!base_imm.has_value() || !add_imm.has_value()) {
      return std::nullopt;
    }
    return MinimalConditionalPhiJoinSlice::IncomingValue{*base_imm, *add_imm};
  };
  if (entry.label != "entry" || entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      true_block.label != entry.terminator.true_label ||
      false_block.label != entry.terminator.false_label ||
      true_block.insts.size() > 1 || false_block.insts.size() > 1 ||
      true_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      false_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      true_block.terminator.target_label != join_block.label ||
      false_block.terminator.target_label != join_block.label ||
      (join_block.insts.size() != 1 && join_block.insts.size() != 2) ||
      join_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !join_block.terminator.value.has_value() || join_block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&join_block.insts.front());
  if (cmp == nullptr || phi == nullptr || cmp->result != entry.terminator.cond_name ||
      cmp->type_str != "i32" || phi->type_str != "i32" || phi->incoming.size() != 2 ||
      phi->incoming[0].label != true_block.label ||
      phi->incoming[1].label != false_block.label) {
    return std::nullopt;
  }

  std::optional<std::int64_t> join_add_imm;
  if (join_block.insts.size() == 1) {
    if (*join_block.terminator.value != phi->result) {
      return std::nullopt;
    }
  } else {
    const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&join_block.insts[1]);
    if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
        add->type_str != "i32" || add->lhs != phi->result ||
        *join_block.terminator.value != add->result) {
      return std::nullopt;
    }
    join_add_imm = parse_i64(add->rhs);
    if (!join_add_imm.has_value()) {
      return std::nullopt;
    }
  }

  const auto lhs_imm = parse_i64(cmp->lhs);
  const auto rhs_imm = parse_i64(cmp->rhs);
  const auto true_value = parse_incoming_value(true_block, phi->incoming[0].value);
  const auto false_value = parse_incoming_value(false_block, phi->incoming[1].value);
  if (!lhs_imm.has_value() || !rhs_imm.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  using BackendPred = c4c::backend::BackendComparePredicate;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;
  LirPred predicate = LirPred::Eq;
  switch (cmp->predicate) {
    case BackendPred::Slt: predicate = LirPred::Slt; break;
    case BackendPred::Sle: predicate = LirPred::Sle; break;
    case BackendPred::Sgt: predicate = LirPred::Sgt; break;
    case BackendPred::Sge: predicate = LirPred::Sge; break;
    case BackendPred::Eq: predicate = LirPred::Eq; break;
    case BackendPred::Ne: predicate = LirPred::Ne; break;
    case BackendPred::Ult: predicate = LirPred::Ult; break;
    case BackendPred::Ule: predicate = LirPred::Ule; break;
    case BackendPred::Ugt: predicate = LirPred::Ugt; break;
    case BackendPred::Uge: predicate = LirPred::Uge; break;
  }

  return MinimalConditionalPhiJoinSlice{predicate,
                                        *lhs_imm,
                                        *rhs_imm,
                                        true_block.label,
                                        false_block.label,
                                        join_block.label,
                                        *true_value,
                                        *false_value,
                                        join_add_imm};
}

std::optional<MinimalLocalArraySlice> parse_minimal_local_array_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "[2 x i32]" || !alloca->count.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> widened_indices;
  std::unordered_map<std::string, std::int64_t> local_ptr_offsets;
  std::vector<std::pair<std::int64_t, std::int64_t>> stores;
  std::vector<std::int64_t> loads;
  std::string add_result;

  for (const auto& inst : entry.insts) {
    if (const auto* gep = std::get_if<LirGepOp>(&inst)) {
      if (gep->element_type == "[2 x i32]" && gep->ptr == alloca->result &&
          gep->indices.size() == 2 && gep->indices[0] == "i64 0" &&
          gep->indices[1] == "i64 0") {
        local_ptr_offsets[gep->result] = 0;
        continue;
      }

      if (gep->element_type == "i32" && gep->indices.size() == 1) {
        const auto base = local_ptr_offsets.find(gep->ptr);
        const auto typed_index = strip_typed_operand_prefix(gep->indices.front(), "i64");
        if (base == local_ptr_offsets.end() || !typed_index.has_value()) {
          return std::nullopt;
        }

        std::int64_t index = 0;
        if (const auto imm = parse_i64(*typed_index); imm.has_value()) {
          index = *imm;
        } else {
          const auto widened = widened_indices.find(std::string(*typed_index));
          if (widened == widened_indices.end()) {
            return std::nullopt;
          }
          index = widened->second;
        }

        if (index < 0 || index > 1) {
          return std::nullopt;
        }
        local_ptr_offsets[gep->result] = base->second + index * 4;
        continue;
      }

      return std::nullopt;
    }

    if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
      if (cast->kind != LirCastKind::SExt || cast->from_type != "i32" ||
          cast->to_type != "i64") {
        return std::nullopt;
      }
      const auto imm = parse_i64(cast->operand);
      if (!imm.has_value()) {
        return std::nullopt;
      }
      widened_indices[cast->result] = *imm;
      continue;
    }

    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str != "i32") {
        return std::nullopt;
      }
      const auto ptr = local_ptr_offsets.find(store->ptr);
      const auto imm = parse_i64(store->val);
      if (ptr == local_ptr_offsets.end() || !imm.has_value()) {
        return std::nullopt;
      }
      stores.emplace_back(ptr->second, *imm);
      continue;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32") {
        return std::nullopt;
      }
      const auto ptr = local_ptr_offsets.find(load->ptr);
      if (ptr == local_ptr_offsets.end()) {
        return std::nullopt;
      }
      loads.push_back(ptr->second);
      continue;
    }

    if (const auto* add = std::get_if<LirBinOp>(&inst)) {
      if (add->opcode != "add" || add->type_str != "i32" || loads.size() != 2) {
        return std::nullopt;
      }
      add_result = add->result;
      continue;
    }

    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32" ||
      *ret->value_str != add_result || stores.size() != 2 || loads.size() != 2) {
    return std::nullopt;
  }

  std::optional<std::int64_t> store0;
  std::optional<std::int64_t> store1;
  for (const auto& [offset, imm] : stores) {
    if (offset == 0) {
      store0 = imm;
    } else if (offset == 4) {
      store1 = imm;
    } else {
      return std::nullopt;
    }
  }
  if (!store0.has_value() || !store1.has_value()) {
    return std::nullopt;
  }

  if (!((loads[0] == 0 && loads[1] == 4) || (loads[0] == 4 && loads[1] == 0))) {
    return std::nullopt;
  }

  return MinimalLocalArraySlice{*store0, *store1};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      !global.init_text.empty()) {
    return std::nullopt;
  }

  const std::string element_prefix = "[";
  const std::string element_suffix = " x i32]";
  if (global.llvm_type.size() <= element_prefix.size() + element_suffix.size() ||
      global.llvm_type.substr(0, element_prefix.size()) != element_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - element_suffix.size()) !=
          element_suffix) {
    return std::nullopt;
  }

  const auto element_count_text = global.llvm_type.substr(
      element_prefix.size(),
      global.llvm_type.size() - element_prefix.size() - element_suffix.size());
  const auto element_count = parse_i64(element_count_text);
  if (!element_count.has_value() || *element_count <= 0) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 4) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
      load == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  if (base_gep->element_type != global.llvm_type || base_gep->ptr != "@" + global.name ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }

  const auto element_index = parse_i64(index_cast->operand);
  if (!element_index.has_value() || *element_index < 0 || *element_index >= *element_count) {
    return std::nullopt;
  }

  if (elem_gep->element_type != "i32" || elem_gep->ptr != base_gep->result ||
      elem_gep->indices.size() != 1 ||
      elem_gep->indices[0] != ("i64 " + index_cast->result)) {
    return std::nullopt;
  }

  if (load->type_str != "i32" || load->ptr != elem_gep->result ||
      !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global.name, *element_index * 4};
}

std::optional<MinimalExternDeclCallSlice> parse_minimal_extern_decl_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.extern_decls.size() != 1 ||
      !module.globals.empty() || !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto& extern_decl = module.extern_decls.front();
  if (extern_decl.name.empty() || extern_decl.name == "main" ||
      extern_decl.return_type_str != "i32") {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.name != "main" || function.is_declaration || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 1 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&entry.insts.front());
  const auto callee_name =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *ret->value_str != call->result || !callee_name.has_value() ||
      *callee_name != extern_decl.name) {
    return std::nullopt;
  }

  return MinimalExternDeclCallSlice{extern_decl.name};
}

std::optional<MinimalExternDeclCallSlice> parse_minimal_declared_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

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

  const auto* call =
      std::get_if<c4c::backend::BackendCallInst>(&main_block.insts.front());
  const auto callee_name =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result || !callee_name.has_value()) {
    return std::nullopt;
  }

  const std::string callee_name_str = std::string(*callee_name);
  if (callee_name_str == "main") {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, callee_name_str);
  if (callee_fn == nullptr || !callee_fn->is_declaration ||
      callee_fn->signature.linkage != "declare" ||
      callee_fn->signature.return_type != "i32" ||
      !callee_fn->signature.params.empty() || callee_fn->signature.is_vararg) {
    return std::nullopt;
  }

  return MinimalExternDeclCallSlice{callee_name_str};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || !global->is_extern_decl || global->qualifier != "global " ||
      global->linkage != "external " || global->llvm_type.size() < 7 ||
      global->llvm_type.substr(global->llvm_type.size() - 7) != " x i32]") {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->address.base_symbol != global->name ||
      *block.terminator.value != load->result || load->address.byte_offset < 0) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global->name, load->address.byte_offset};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || global->is_extern_decl || global->qualifier != "global " ||
      global->llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global->init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global->name, *init_imm};
}

std::optional<MinimalGlobalCharPointerDiffSlice> parse_minimal_global_char_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || global->is_extern_decl || global->qualifier != "global ") {
    return std::nullopt;
  }
  const std::string array_prefix = "[";
  const std::string array_suffix = " x i8]";
  if (global->llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global->llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global->llvm_type.substr(global->llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global->llvm_type.substr(
      array_prefix.size(),
      global->llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr || ptrdiff->type_str != "i32" ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 || ptrdiff->element_size != 1 ||
      ptrdiff->expected_diff != ptrdiff->lhs_address.byte_offset ||
      ptrdiff->lhs_address.byte_offset <= 0 ||
      ptrdiff->lhs_address.byte_offset >= *global_size ||
      *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalCharPointerDiffSlice{
      global->name,
      *global_size,
      ptrdiff->lhs_address.byte_offset,
  };
}

std::optional<MinimalGlobalIntPointerDiffSlice> parse_minimal_global_int_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || global->is_extern_decl || global->qualifier != "global ") {
    return std::nullopt;
  }
  const std::string array_prefix = "[";
  const std::string array_suffix = " x i32]";
  if (global->llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global->llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global->llvm_type.substr(global->llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global->llvm_type.substr(
      array_prefix.size(),
      global->llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr || ptrdiff->type_str != "i32" ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 || ptrdiff->expected_diff != 1 ||
      ptrdiff->lhs_address.byte_offset != ptrdiff->element_size ||
      ptrdiff->element_size != 4 || *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalIntPointerDiffSlice{global->name, *global_size, 4, 2};
}

std::optional<MinimalGlobalCharPointerDiffSlice> parse_minimal_global_char_pointer_diff_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global ") {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i8]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }

  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 12) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep1 = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* byte_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[9]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[10]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[11]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || byte_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || byte_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      expected_diff == nullptr || cmp == nullptr || extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep1->element_type != global.llvm_type || base_gep1->ptr != global_ptr ||
      base_gep1->indices.size() != 2 || base_gep1->indices[0] != "i64 0" ||
      base_gep1->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (base_gep0->element_type != global.llvm_type || base_gep0->ptr != global_ptr ||
      base_gep0->indices.size() != 2 || base_gep0->indices[0] != "i64 0" ||
      base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index1->operand);
  if (!byte_index.has_value() || *byte_index <= 0 || *byte_index >= *global_size) {
    return std::nullopt;
  }

  if (index0->kind != LirCastKind::SExt || index0->from_type != "i32" ||
      index0->to_type != "i64" || index0->operand != "0") {
    return std::nullopt;
  }

  if (byte_gep1->element_type != "i8" || byte_gep1->ptr != base_gep1->result ||
      byte_gep1->indices.size() != 1 ||
      byte_gep1->indices[0] != ("i64 " + index1->result)) {
    return std::nullopt;
  }
  if (byte_gep0->element_type != "i8" || byte_gep0->ptr != base_gep0->result ||
      byte_gep0->indices.size() != 1 ||
      byte_gep0->indices[0] != ("i64 " + index0->result)) {
    return std::nullopt;
  }

  if (ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != byte_gep1->result || ptrtoint1->to_type != "i64") {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != byte_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  const auto diff_opcode = diff->opcode.typed();
  if (diff_opcode != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result ||
      diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64") {
    return std::nullopt;
  }
  const auto expected_value = parse_i64(expected_diff->operand);
  if (!expected_value.has_value() || *expected_value != *byte_index) {
    return std::nullopt;
  }

  const auto cmp_predicate = cmp->predicate.typed();
  if (cmp->is_float || cmp_predicate != LirCmpPredicate::Eq || cmp->type_str != "i64" ||
      cmp->lhs != diff->result ||
      cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }

  if (extend->kind != LirCastKind::ZExt || extend->from_type != "i1" ||
      extend->operand != cmp->result || extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalGlobalCharPointerDiffSlice{global.name, *global_size, *byte_index};
}

std::optional<MinimalGlobalIntPointerDiffSlice> parse_minimal_global_int_pointer_diff_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.align_bytes != 4 ||
      (global.init_text != "zeroinitializer" && global.init_text != "[i32 0, i32 0]")) {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i32]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }

  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size != 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 13) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* elem_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* scaled_diff = std::get_if<LirBinOp>(&entry.insts[9]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[10]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[11]);
  const auto* final_extend = std::get_if<LirCastOp>(&entry.insts[12]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index1 == nullptr || elem_gep == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || elem_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      scaled_diff == nullptr || expected_diff == nullptr || cmp == nullptr ||
      final_extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep->element_type != global.llvm_type || base_gep->ptr != global_ptr ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (elem_gep->element_type != "i32" || elem_gep->ptr != base_gep->result ||
      elem_gep->indices.size() != 1 ||
      elem_gep->indices[0] != ("i64 " + index1->result)) {
    return std::nullopt;
  }
  if (base_gep0->element_type != global.llvm_type || base_gep0->ptr != global_ptr ||
      base_gep0->indices.size() != 2 || base_gep0->indices[0] != "i64 0" ||
      base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }
  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64" || index1->operand != "1") {
    return std::nullopt;
  }
  if (index0->kind != LirCastKind::SExt || index0->from_type != "i32" ||
      index0->to_type != "i64" || index0->operand != "0") {
    return std::nullopt;
  }
  if (elem_gep0->element_type != "i32" || elem_gep0->ptr != base_gep0->result ||
      elem_gep0->indices.size() != 1 ||
      elem_gep0->indices[0] != ("i64 " + index0->result)) {
    return std::nullopt;
  }

  if (ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != elem_gep->result || ptrtoint1->to_type != "i64") {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != elem_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  const auto diff_opcode = diff->opcode.typed();
  if (diff_opcode != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result ||
      diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }
  const auto scaled_diff_opcode = scaled_diff->opcode.typed();
  if (scaled_diff_opcode != LirBinaryOpcode::SDiv || scaled_diff->type_str != "i64" ||
      scaled_diff->lhs != diff->result || scaled_diff->rhs != "4") {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64" || expected_diff->operand != "1") {
    return std::nullopt;
  }

  const auto cmp_predicate = cmp->predicate.typed();
  if (cmp->is_float || cmp_predicate != LirCmpPredicate::Eq || cmp->type_str != "i64" ||
      cmp->lhs != scaled_diff->result ||
      cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }

  if (final_extend->kind != LirCastKind::ZExt || final_extend->from_type != "i1" ||
      final_extend->operand != cmp->result || final_extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != final_extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalGlobalIntPointerDiffSlice{global.name, *global_size, 4, 2};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_global_int_pointer_roundtrip_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* addr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (addr_slot == nullptr || ptr_slot == nullptr || addr_slot->result == ptr_slot->result ||
      addr_slot->type_str != "i64" || ptr_slot->type_str != "ptr" ||
      addr_slot->align != 8 || ptr_slot->align != 8) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 7) {
    return std::nullopt;
  }

  const auto* ptrtoint = std::get_if<LirCastOp>(&entry.insts[0]);
  const auto* spill_addr = std::get_if<LirStoreOp>(&entry.insts[1]);
  const auto* reload_addr = std::get_if<LirLoadOp>(&entry.insts[2]);
  const auto* inttoptr = std::get_if<LirCastOp>(&entry.insts[3]);
  const auto* spill_ptr = std::get_if<LirStoreOp>(&entry.insts[4]);
  const auto* reload_ptr = std::get_if<LirLoadOp>(&entry.insts[5]);
  const auto* load_value = std::get_if<LirLoadOp>(&entry.insts[6]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ptrtoint == nullptr || spill_addr == nullptr || reload_addr == nullptr ||
      inttoptr == nullptr || spill_ptr == nullptr || reload_ptr == nullptr ||
      load_value == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (ptrtoint->kind != LirCastKind::PtrToInt || ptrtoint->from_type != "ptr" ||
      ptrtoint->operand != global_ptr || ptrtoint->to_type != "i64") {
    return std::nullopt;
  }
  if (spill_addr->type_str != "i64" || spill_addr->val != ptrtoint->result ||
      spill_addr->ptr != addr_slot->result) {
    return std::nullopt;
  }
  if (reload_addr->type_str != "i64" || reload_addr->ptr != addr_slot->result) {
    return std::nullopt;
  }
  if (inttoptr->kind != LirCastKind::IntToPtr || inttoptr->from_type != "i64" ||
      inttoptr->operand != reload_addr->result || inttoptr->to_type != "ptr") {
    return std::nullopt;
  }
  if (spill_ptr->type_str != "ptr" || spill_ptr->val != inttoptr->result ||
      spill_ptr->ptr != ptr_slot->result) {
    return std::nullopt;
  }
  if (reload_ptr->type_str != "ptr" || reload_ptr->ptr != ptr_slot->result) {
    return std::nullopt;
  }
  if (load_value->type_str != "i32" || load_value->ptr != reload_ptr->result) {
    return std::nullopt;
  }
  if (!ret->value_str.has_value() || *ret->value_str != load_value->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 1 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->ptr != ("@" + global.name) || *ret->value_str != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm};
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global.init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 2 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&entry.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[1]);
  if (store == nullptr || load == nullptr || store->type_str != "i32" ||
      load->type_str != "i32" || store->ptr != ("@" + global.name) ||
      load->ptr != store->ptr || *ret->value_str != load->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->val);
  if (!store_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{global.name, *init_imm, *store_imm};
}

std::optional<MinimalStringLiteralCharSlice> parse_minimal_string_literal_char_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.size() != 1 ||
      !module.globals.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& string_const = module.string_pool.front();
  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || byte_gep == nullptr ||
      load == nullptr || extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  const auto string_array_type =
      "[" + std::to_string(string_const.byte_length) + " x i8]";
  if (base_gep->element_type != string_array_type || base_gep->ptr != string_const.pool_name ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index_cast->operand);
  if (!byte_index.has_value() || *byte_index < 0 ||
      *byte_index >= static_cast<std::int64_t>(string_const.byte_length)) {
    return std::nullopt;
  }

  if (byte_gep->element_type != "i8" || byte_gep->ptr != base_gep->result ||
      byte_gep->indices.size() != 1 || byte_gep->indices[0] != ("i64 " + index_cast->result)) {
    return std::nullopt;
  }

  if (load->type_str != "i8" || load->ptr != byte_gep->result) {
    return std::nullopt;
  }

  if ((extend->kind != LirCastKind::SExt && extend->kind != LirCastKind::ZExt) ||
      extend->from_type != "i8" || extend->operand != load->result ||
      extend->to_type != "i32") {
    return std::nullopt;
  }

  if (!ret->value_str.has_value() || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      string_const.pool_name,
      string_const.raw_bytes,
      *byte_index,
      extend->kind == LirCastKind::SExt,
  };
}

std::optional<MinimalStringLiteralCharSlice> parse_minimal_string_literal_char_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.string_constants.size() != 1 ||
      !module.globals.empty()) {
    return std::nullopt;
  }

  const auto* string_const =
      find_string_constant(module, module.string_constants.front().name);
  const auto* main_fn = find_function(module, "main");
  if (string_const == nullptr || main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->result != *block.terminator.value ||
      load->type_str != "i32" || load->memory_type != "i8" ||
      load->address.base_symbol != string_const->name ||
      load->address.byte_offset < 0 ||
      load->address.byte_offset >=
          static_cast<std::int64_t>(string_const->byte_length)) {
    return std::nullopt;
  }
  if (load->extension != c4c::backend::BackendLoadExtension::SignExtend &&
      load->extension != c4c::backend::BackendLoadExtension::ZeroExtend) {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      "@" + string_const->name,
      string_const->raw_bytes,
      load->address.byte_offset,
      load->extension == c4c::backend::BackendLoadExtension::SignExtend,
  };
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || global->is_extern_decl || global->qualifier != "global " ||
      global->llvm_type != "i32") {
    return std::nullopt;
  }
  const auto init_imm = parse_i64(global->init_text);
  if (!init_imm.has_value()) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      main_fn->signature.linkage != "define" ||
      main_fn->signature.return_type != "i32" ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 2 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* store = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[0]);
  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[1]);
  if (store == nullptr || load == nullptr || store->type_str != "i32" ||
      load->type_str != "i32" || store->address.base_symbol != global->name ||
      store->address.byte_offset != 0 || load->address.base_symbol != global->name ||
      load->address.byte_offset != 0 || *block.terminator.value != load->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->value);
  if (!store_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{global->name, *init_imm, *store_imm};
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
  const auto call_operand =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_direct_global_single_typed_call_operand(
                            *call, "add_one", "i32");
  if (source_add == nullptr || call == nullptr || final_add == nullptr ||
      source_add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      source_add->type_str != "i32" || call->return_type != "i32" ||
      !call_operand.has_value() || *call_operand != source_add->result ||
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
  const auto callee_name =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result || !callee_name.has_value()) {
    return std::nullopt;
  }

  if (*callee_name == "main") return std::nullopt;

  const auto* callee_fn = find_function(module, *callee_name);
  if (callee_fn == nullptr) return std::nullopt;
  const auto callee_imm = parse_single_block_return_imm(*callee_fn);
  if (!callee_imm.has_value()) return std::nullopt;

  return MinimalDirectCallSlice{std::string(*callee_name), *callee_imm};
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
  const auto call_operand =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_direct_global_single_typed_call_operand(
                            *call, "add_one", "i32");
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result || !call_operand.has_value()) {
    return std::nullopt;
  }

  const auto arg_imm = parse_i64(*call_operand);
  if (!arg_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, "add_one");
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

  return MinimalDirectCallAddImmSlice{
      "add_one",
      *arg_imm,
      *add_imm,
  };
}

std::optional<MinimalParamSlotSlice> parse_minimal_param_slot_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto* helper = find_lir_function(module, "add_one");
  const auto* main_fn = find_lir_function(module, "main");
  if (helper == nullptr || main_fn == nullptr || helper->is_declaration ||
      main_fn->is_declaration || helper->signature_text != "define i32 @add_one(i32 %p.x)\n" ||
      main_fn->signature_text != "define i32 @main()\n" || helper->entry.value != 0 ||
      main_fn->entry.value != 0 || helper->blocks.size() != 1 || main_fn->blocks.size() != 1 ||
      helper->alloca_insts.size() != 2 || !main_fn->alloca_insts.empty() ||
      !helper->stack_objects.empty() || !main_fn->stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&helper->alloca_insts[0]);
  const auto* arg_store = std::get_if<LirStoreOp>(&helper->alloca_insts[1]);
  if (alloca == nullptr || arg_store == nullptr || alloca->result != "%lv.param.x" ||
      alloca->type_str != "i32" || !alloca->count.empty() || arg_store->type_str != "i32" ||
      arg_store->val != "%p.x" || arg_store->ptr != "%lv.param.x") {
    return std::nullopt;
  }

  const auto& helper_block = helper->blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
  if (helper_block.label != "entry" || helper_block.insts.size() != 4 || helper_ret == nullptr ||
      !helper_ret->value_str.has_value() || helper_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* load0 = std::get_if<LirLoadOp>(&helper_block.insts[0]);
  const auto* add = std::get_if<LirBinOp>(&helper_block.insts[1]);
  const auto* store = std::get_if<LirStoreOp>(&helper_block.insts[2]);
  const auto* load1 = std::get_if<LirLoadOp>(&helper_block.insts[3]);
  const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
  if (load0 == nullptr || add == nullptr || store == nullptr || load1 == nullptr ||
      load0->type_str != "i32" || load0->ptr != "%lv.param.x" ||
      add_opcode != LirBinaryOpcode::Add || add->type_str != "i32" ||
      add->lhs != load0->result ||
      store->type_str != "i32" || store->val != add->result || store->ptr != "%lv.param.x" ||
      load1->type_str != "i32" || load1->ptr != "%lv.param.x" ||
      *helper_ret->value_str != load1->result) {
    return std::nullopt;
  }

  const auto helper_add_imm = parse_i64(add->rhs);
  if (!helper_add_imm.has_value()) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
  if (main_block.label != "entry" || main_block.insts.size() != 1 || main_ret == nullptr ||
      !main_ret->value_str.has_value() || main_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&main_block.insts.front());
  const auto call_operand =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_direct_global_single_typed_call_operand(
                            *call, "add_one", "i32");
  if (call == nullptr || call->result.empty() || !call_operand.has_value() ||
      *main_ret->value_str != call->result) {
    return std::nullopt;
  }

  const auto call_arg_imm = parse_i64(*call_operand);
  if (!call_arg_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalParamSlotSlice{
      "add_one",
      *call_arg_imm,
      *helper_add_imm,
  };
}

std::optional<MinimalTwoArgDirectCallSlice> parse_minimal_two_arg_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto* helper = find_lir_function(module, "add_pair");
  const auto* main_fn = find_lir_function(module, "main");
  if (helper == nullptr || main_fn == nullptr || helper->is_declaration ||
      main_fn->is_declaration ||
      helper->signature_text != "define i32 @add_pair(i32 %p.x, i32 %p.y)\n" ||
      main_fn->signature_text != "define i32 @main()\n" || helper->entry.value != 0 ||
      main_fn->entry.value != 0 || helper->blocks.size() != 1 || main_fn->blocks.size() != 1 ||
      !helper->alloca_insts.empty() || !helper->stack_objects.empty() ||
      !main_fn->stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& helper_block = helper->blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
  if (helper_block.label != "entry" || helper_block.insts.size() != 1 || helper_ret == nullptr ||
      !helper_ret->value_str.has_value() || helper_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<LirBinOp>(&helper_block.insts.front());
  const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
  if (add == nullptr || add_opcode != LirBinaryOpcode::Add || add->type_str != "i32" ||
      add->lhs != "%p.x" || add->rhs != "%p.y" || *helper_ret->value_str != add->result) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
  if (main_block.label != "entry" || main_ret == nullptr ||
      !main_ret->value_str.has_value() || main_ret->type_str != "i32") {
    return std::nullopt;
  }

  if (main_fn->alloca_insts.empty()) {
    if (main_block.insts.size() != 1) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.front());
    const auto call_operands =
        call == nullptr ? std::nullopt
                        : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                              *call, "add_pair", "i32", "i32");
    if (call == nullptr || call->result.empty() || !call_operands.has_value() ||
        *main_ret->value_str != call->result) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(call_operands->first);
    const auto rhs_call_arg_imm = parse_i64(call_operands->second);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return MinimalTwoArgDirectCallSlice{
        "add_pair",
        *lhs_call_arg_imm,
        *rhs_call_arg_imm,
    };
  }

  if (main_fn->alloca_insts.size() != 1) {
    if (main_fn->alloca_insts.size() != 2) {
      return std::nullopt;
    }

    const auto* alloca0 = std::get_if<LirAllocaOp>(&main_fn->alloca_insts[0]);
    const auto* alloca1 = std::get_if<LirAllocaOp>(&main_fn->alloca_insts[1]);
    if (alloca0 == nullptr || alloca1 == nullptr || alloca0->type_str != "i32" ||
        alloca1->type_str != "i32" || !alloca0->count.empty() || !alloca1->count.empty()) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.back());
    const auto call_operands =
        call == nullptr ? std::nullopt
                        : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                              *call, "add_pair", "i32", "i32");
    if (call == nullptr || call->result.empty() || !call_operands.has_value() ||
        *main_ret->value_str != call->result) {
      return std::nullopt;
    }

    struct SlotState {
      std::string alloca_name;
      std::int64_t stored_imm = 0;
      bool initialized = false;
      std::string last_load_result;
      std::string rewritten_result;
      bool rewrite_committed = false;
    };

    auto parse_store_imm = [](const LirInst& inst, std::string_view expected_ptr)
        -> std::optional<std::int64_t> {
      const auto* store = std::get_if<LirStoreOp>(&inst);
      if (store == nullptr || store->type_str != "i32" || store->ptr != expected_ptr) {
        return std::nullopt;
      }
      return parse_i64(store->val);
    };

    SlotState lhs_slot{alloca0->result};
    SlotState rhs_slot{alloca1->result};
    const auto lhs_store_imm = parse_store_imm(main_block.insts[0], lhs_slot.alloca_name);
    const auto rhs_store_imm = parse_store_imm(main_block.insts[1], rhs_slot.alloca_name);
    if (!lhs_store_imm.has_value() || !rhs_store_imm.has_value()) {
      return std::nullopt;
    }
    lhs_slot.stored_imm = *lhs_store_imm;
    lhs_slot.initialized = true;
    rhs_slot.stored_imm = *rhs_store_imm;
    rhs_slot.initialized = true;

    auto match_slot = [&](std::string_view ptr) -> SlotState* {
      if (ptr == lhs_slot.alloca_name) return &lhs_slot;
      if (ptr == rhs_slot.alloca_name) return &rhs_slot;
      return nullptr;
    };

    for (std::size_t inst_index = 2; inst_index + 1 < main_block.insts.size(); ++inst_index) {
      const auto& inst = main_block.insts[inst_index];

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        if (load->type_str != "i32") {
          return std::nullopt;
        }
        auto* slot = match_slot(load->ptr);
        if (slot == nullptr || !slot->initialized) {
          return std::nullopt;
        }
        slot->last_load_result = load->result;
        continue;
      }

      if (const auto* rewrite = std::get_if<LirBinOp>(&inst)) {
        if (rewrite->opcode != "add" || rewrite->type_str != "i32" || rewrite->rhs != "0") {
          return std::nullopt;
        }
        SlotState* matched_slot = nullptr;
        for (auto* slot : {&lhs_slot, &rhs_slot}) {
          if (slot->last_load_result == rewrite->lhs) {
            matched_slot = slot;
            break;
          }
        }
        if (matched_slot == nullptr) {
          return std::nullopt;
        }
        matched_slot->rewritten_result = rewrite->result;
        continue;
      }

      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        if (store->type_str != "i32") {
          return std::nullopt;
        }
        auto* slot = match_slot(store->ptr);
        if (slot == nullptr || store->val != slot->rewritten_result) {
          return std::nullopt;
        }
        slot->rewrite_committed = true;
        continue;
      }

      return std::nullopt;
    }

    if (call_operands->first != lhs_slot.last_load_result ||
        call_operands->second != rhs_slot.last_load_result) {
      return std::nullopt;
    }

    return MinimalTwoArgDirectCallSlice{
        "add_pair",
        lhs_slot.stored_imm,
        rhs_slot.stored_imm,
    };
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&main_fn->alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "i32" || !alloca->count.empty()) {
    return std::nullopt;
  }

  if (main_block.insts.size() == 3) {
    const auto* store = std::get_if<LirStoreOp>(&main_block.insts[0]);
    const auto* load = std::get_if<LirLoadOp>(&main_block.insts[1]);
    const auto* call = std::get_if<LirCallOp>(&main_block.insts[2]);
    const auto call_operands =
        call == nullptr ? std::nullopt
                        : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                              *call, "add_pair", "i32", "i32");
    if (store == nullptr || load == nullptr || call == nullptr || store->type_str != "i32" ||
        load->type_str != "i32" || store->ptr != alloca->result ||
        load->ptr != alloca->result || call->result.empty() || !call_operands.has_value() ||
        *main_ret->value_str != call->result) {
      return std::nullopt;
    }
    const auto stored_imm = parse_i64(store->val);
    if (!stored_imm.has_value()) {
      return std::nullopt;
    }
    if (call_operands->first == load->result) {
      const auto rhs_call_arg_imm = parse_i64(call_operands->second);
      if (!rhs_call_arg_imm.has_value()) {
        return std::nullopt;
      }
      return MinimalTwoArgDirectCallSlice{"add_pair", *stored_imm, *rhs_call_arg_imm};
    }
    if (call_operands->second == load->result) {
      const auto lhs_call_arg_imm = parse_i64(call_operands->first);
      if (!lhs_call_arg_imm.has_value()) {
        return std::nullopt;
      }
      return MinimalTwoArgDirectCallSlice{"add_pair", *lhs_call_arg_imm, *stored_imm};
    }
    return std::nullopt;
  }

  if (main_block.insts.size() != 6) {
    return std::nullopt;
  }

  const auto* store0 = std::get_if<LirStoreOp>(&main_block.insts[0]);
  const auto* load0 = std::get_if<LirLoadOp>(&main_block.insts[1]);
  const auto* rewrite = std::get_if<LirBinOp>(&main_block.insts[2]);
  const auto* store1 = std::get_if<LirStoreOp>(&main_block.insts[3]);
  const auto* load1 = std::get_if<LirLoadOp>(&main_block.insts[4]);
  const auto* call = std::get_if<LirCallOp>(&main_block.insts[5]);
  const auto call_operands =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                            *call, "add_pair", "i32", "i32");
  const auto rewrite_opcode = rewrite == nullptr ? std::nullopt : rewrite->opcode.typed();
  if (store0 == nullptr || load0 == nullptr || rewrite == nullptr || store1 == nullptr ||
      load1 == nullptr || call == nullptr || store0->type_str != "i32" ||
      load0->type_str != "i32" || rewrite_opcode != LirBinaryOpcode::Add ||
      rewrite->type_str != "i32" ||
      store1->type_str != "i32" || load1->type_str != "i32" ||
      store0->ptr != alloca->result || load0->ptr != alloca->result ||
      store1->ptr != alloca->result || load1->ptr != alloca->result ||
      rewrite->lhs != load0->result || rewrite->rhs != "0" || store1->val != rewrite->result ||
      call->result.empty() || !call_operands.has_value() ||
      *main_ret->value_str != call->result) {
    return std::nullopt;
  }

  const auto stored_imm = parse_i64(store0->val);
  if (!stored_imm.has_value()) {
    return std::nullopt;
  }

  if (call_operands->first == load1->result) {
    const auto rhs_call_arg_imm = parse_i64(call_operands->second);
    if (!rhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }
    return MinimalTwoArgDirectCallSlice{"add_pair", *stored_imm, *rhs_call_arg_imm};
  }
  if (call_operands->second == load1->result) {
    const auto lhs_call_arg_imm = parse_i64(call_operands->first);
    if (!lhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }
    return MinimalTwoArgDirectCallSlice{"add_pair", *lhs_call_arg_imm, *stored_imm};
  }
  return std::nullopt;
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

std::string emit_minimal_direct_call_asm(const c4c::backend::BackendModule& module,
                                         const MinimalDirectCallSlice& slice) {
  if (slice.callee_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.callee_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "helper return immediates outside the minimal mov-supported range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, " << slice.callee_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::BackendModule& module,
    const MinimalDirectCallAddImmSlice& slice) {
  if (slice.arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "single-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_param_slot_asm(const c4c::backend::BackendModule& module,
                                        const MinimalParamSlotSlice& slice) {
  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.helper_add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.helper_add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "parameter-slot immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.helper_add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const MinimalTwoArgDirectCallSlice& slice) {
  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "two-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, esi\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
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
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
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

std::string emit_minimal_countdown_loop_asm(const c4c::backend::BackendModule& module,
                                            const MinimalCountdownLoopSlice& slice) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << slice.initial_imm << "\n";
  out << ".L" << slice.loop_label << ":\n";
  out << "  cmp eax, 0\n";
  out << "  je .L" << slice.exit_label << "\n";
  out << ".L" << slice.body_label << ":\n";
  out << "  sub eax, 1\n";
  out << "  jmp .L" << slice.loop_label << "\n";
  out << ".L" << slice.exit_label << ":\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_phi_join_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalPhiJoinSlice& slice) {
  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";

  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "jge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "jg"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "jle"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "jl"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "jne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "je"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "jae"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "ja"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "jbe"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "jb"; break;
    default:
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          "conditional-phi-join predicates outside the current compare-and-branch x86 slice");
  }

  out << "  mov eax, " << slice.lhs_imm << "\n";
  out << "  cmp eax, " << slice.rhs_imm << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  out << "  mov eax, " << slice.true_value.base_imm << "\n";
  if (slice.true_value.add_imm.has_value()) {
    out << "  add eax, " << *slice.true_value.add_imm << "\n";
  }
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  out << "  mov eax, " << slice.false_value.base_imm << "\n";
  if (slice.false_value.add_imm.has_value()) {
    out << "  add eax, " << *slice.false_value.add_imm << "\n";
  }
  out << ".L" << slice.join_label << ":\n";
  if (slice.join_add_imm.has_value()) {
    out << "  add eax, " << *slice.join_add_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_local_array_asm(const c4c::backend::BackendModule& module,
                                         const MinimalLocalArraySlice& slice) {
  if (slice.store0_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store0_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store1_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store1_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "local-array store immediates outside the minimal mov-supported range");
  }

  const std::string symbol = asm_symbol_name(module, "main");
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  lea rcx, [rbp - 8]\n"
      << "  mov dword ptr [rcx], " << slice.store0_imm << "\n"
      << "  mov dword ptr [rcx + 4], " << slice.store1_imm << "\n"
      << "  mov eax, dword ptr [rcx]\n"
      << "  add eax, dword ptr [rcx + 4]\n"
      << "  mov rsp, rbp\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_global_array_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalExternGlobalArrayLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax + " << slice.byte_offset << "]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_decl_call_asm(
    const c4c::backend::BackendModule& module,
    const MinimalExternDeclCallSlice& slice) {
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_char_pointer_diff_asm(
    const c4c::backend::BackendModule& module,
    const MinimalGlobalCharPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".bss\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .zero " << slice.global_size << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  lea rcx, [rax + " << slice.byte_offset << "]\n"
      << "  sub rcx, rax\n"
      << "  cmp rcx, " << slice.byte_offset << "\n"
      << "  sete al\n"
      << "  movzx eax, al\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_int_pointer_diff_asm(
    const c4c::backend::BackendModule& module,
    const MinimalGlobalIntPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".bss\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .zero " << (slice.global_size * 4) << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  lea rcx, [rax + " << slice.byte_offset << "]\n"
      << "  sub rcx, rax\n"
      << "  sar rcx, " << slice.element_shift << "\n"
      << "  cmp rcx, 1\n"
      << "  sete al\n"
      << "  movzx eax, al\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalScalarGlobalLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".data\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_asm(
    const c4c::backend::BackendModule& module,
    const MinimalScalarGlobalStoreReloadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".data\n"
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.store_imm << "\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalStringLiteralCharSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const std::string string_label = asm_private_data_label(module, slice.pool_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".section .rodata\n";
  out << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << string_label << "[rip]\n"
      << "  " << (slice.sign_extend ? "movsx" : "movzx")
      << " eax, byte ptr [rax + " << slice.byte_offset << "]\n"
      << "  ret\n";
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

std::string emit_module(const c4c::backend::BackendModule& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  try {
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_global_array_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_declared_direct_call_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_decl_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_store_reload_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_char_pointer_diff_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_int_pointer_diff_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_string_literal_char_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_return_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(module);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_slice(module);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_add_imm_slice(module);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module);
        slice.has_value()) {
      if (legacy_fallback == nullptr) {
        throw c4c::backend::LirAdapterError(
            c4c::backend::LirAdapterErrorKind::Unsupported,
            "shared x86 call-crossing slices still require legacy LIR fallback");
      }
      const auto* main_fn = find_lir_function(*legacy_fallback, "main");
      if (main_fn == nullptr) {
        throw c4c::backend::LirAdapterError(
            c4c::backend::LirAdapterErrorKind::Unsupported,
            "main function for shared x86 call-crossing direct-call slice");
      }
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          module, run_shared_x86_regalloc(*main_fn), *slice));
    }
    if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  if (legacy_fallback != nullptr) {
    return emit_module(*legacy_fallback);
  }
  return c4c::backend::print_backend_ir(module);
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  try {
    if (const auto slice = parse_minimal_param_slot_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_param_slot_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_two_arg_direct_call_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_two_arg_direct_call_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_local_array_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_local_array_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_extern_decl_call_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_extern_decl_call_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_extern_global_array_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_char_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_int_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_roundtrip_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_store_reload_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      return emit_minimal_string_literal_char_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      c4c::backend::BackendModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_conditional_return_asm(scaffold_module, *slice);
    }
    const auto adapted = c4c::backend::adapt_minimal_module(module);
    if (const auto slice = parse_minimal_declared_direct_call_slice(adapted);
        slice.has_value()) {
      return emit_minimal_extern_decl_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_add_imm_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(adapted, *slice);
    }
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
    if (const auto slice = parse_minimal_countdown_loop_slice(adapted);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(adapted);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(adapted, *slice);
    }
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(adapted); imm.has_value()) {
      return emit_minimal_return_asm(adapted, *imm);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  return c4c::codegen::lir::print_llvm(module);
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(
      assembler::AssembleRequest{.asm_text = emit_module(module), .output_path = output_path});
}

}  // namespace c4c::backend::x86
