#include "x86_codegen.hpp"
#include "peephole/peephole.hpp"

#include "../../backend.hpp"
#include "../../bir.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../lowering/lir_to_bir.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <cctype>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace c4c::backend::x86 {

namespace {

struct MinimalCountdownLoopSlice {
  std::string function_name;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
  std::int64_t initial_imm = 0;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  std::size_t align_bytes = 4;
};

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

struct MinimalTwoFunctionImmediateReturnSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t helper_imm = 0;
  std::int64_t entry_imm = 0;
};

struct MinimalThreeFunctionImmediateReturnSlice {
  std::string first_name;
  std::string second_name;
  std::string entry_name;
  std::int64_t first_imm = 0;
  std::int64_t second_imm = 0;
  std::int64_t entry_imm = 0;
};

struct MinimalLocalTempSlice {
  std::string function_name;
  std::int64_t stored_imm = 0;
};

struct MinimalConstantBranchReturnSlice {
  std::string function_name;
  std::int64_t returned_imm = 0;
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

std::optional<std::int64_t> parse_i64(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return value.immediate;
}

std::optional<MinimalCountdownLoopSlice> parse_minimal_countdown_loop_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || function.local_slots.size() != 1 ||
      function.blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& slot = function.local_slots.front();
  if (slot.name.empty() || slot.type != TypeKind::I32 || slot.size_bytes != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& loop = function.blocks[1];
  const auto& body = function.blocks[2];
  const auto& exit = function.blocks[3];

  const auto* entry_store =
      entry.insts.size() == 1 ? std::get_if<StoreLocalInst>(&entry.insts[0]) : nullptr;
  const auto* loop_load =
      loop.insts.size() == 2 ? std::get_if<LoadLocalInst>(&loop.insts[0]) : nullptr;
  const auto* loop_cmp =
      loop.insts.size() == 2 ? std::get_if<BinaryInst>(&loop.insts[1]) : nullptr;
  const auto* body_load =
      body.insts.size() == 3 ? std::get_if<LoadLocalInst>(&body.insts[0]) : nullptr;
  const auto* body_sub =
      body.insts.size() == 3 ? std::get_if<BinaryInst>(&body.insts[1]) : nullptr;
  const auto* body_store =
      body.insts.size() == 3 ? std::get_if<StoreLocalInst>(&body.insts[2]) : nullptr;
  const auto* exit_load =
      exit.insts.size() == 1 ? std::get_if<LoadLocalInst>(&exit.insts[0]) : nullptr;
  if (entry.label != "entry" || entry_store == nullptr ||
      entry.terminator.kind != TerminatorKind::Branch ||
      entry_store->slot_name != slot.name ||
      entry_store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      entry_store->value.type != TypeKind::I32 ||
      entry.terminator.target_label != loop.label || loop_load == nullptr ||
      loop_cmp == nullptr || loop.terminator.kind != TerminatorKind::CondBranch ||
      loop_load->slot_name != slot.name ||
      loop_load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      loop_load->result.type != TypeKind::I32 || loop_cmp->opcode != BinaryOpcode::Ne ||
      loop_cmp->result.kind != c4c::backend::bir::Value::Kind::Named ||
      loop_cmp->result.type != TypeKind::I32 || loop_cmp->lhs != loop_load->result ||
      loop_cmp->rhs != c4c::backend::bir::Value::immediate_i32(0) ||
      loop.terminator.condition != loop_cmp->result ||
      loop.terminator.true_label != body.label ||
      loop.terminator.false_label != exit.label || body_load == nullptr ||
      body_sub == nullptr || body_store == nullptr ||
      body.terminator.kind != TerminatorKind::Branch ||
      body_load->slot_name != slot.name ||
      body_load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      body_load->result.type != TypeKind::I32 || body_sub->opcode != BinaryOpcode::Sub ||
      body_sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      body_sub->result.type != TypeKind::I32 || body_sub->lhs != body_load->result ||
      body_sub->rhs != c4c::backend::bir::Value::immediate_i32(1) ||
      body_store->slot_name != slot.name || body_store->value != body_sub->result ||
      body.terminator.target_label != loop.label || exit_load == nullptr ||
      exit.terminator.kind != TerminatorKind::Return ||
      exit_load->slot_name != slot.name ||
      exit_load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      exit_load->result.type != TypeKind::I32 || !exit.terminator.value.has_value() ||
      *exit.terminator.value != exit_load->result) {
    return std::nullopt;
  }

  if (entry_store->value.immediate < 0) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{
      .function_name = function.name,
      .loop_label = loop.label,
      .body_label = body.label,
      .exit_label = exit.label,
      .initial_imm = entry_store->value.immediate,
  };
}

std::string decode_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

const c4c::backend::bir::BinaryInst* get_binary_inst(
    const c4c::backend::bir::Inst& inst) {
  return std::get_if<c4c::backend::bir::BinaryInst>(&inst);
}

bool is_minimal_single_function_asm_slice(const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1 || !module.globals.empty()) {
    return false;
  }
  const auto& function = module.functions.front();
  return function.params.empty() && function.blocks.size() == 1 &&
         function.blocks.front().label == "entry" &&
         function.return_type == c4c::backend::bir::TypeKind::I32;
}

std::optional<AffineValue> lower_affine_operand(
    const c4c::backend::bir::Value& operand,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (const auto imm = parse_i64(operand); imm.has_value()) {
    return AffineValue{false, false, *imm};
  }
  if (operand.kind != c4c::backend::bir::Value::Kind::Named ||
      operand.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (!param_names.empty() && operand.name == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && operand.name == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(operand.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<AffineValue> combine_affine_values(const AffineValue& lhs,
                                                 const AffineValue& rhs,
                                                 c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      if ((lhs.uses_first_param && rhs.uses_first_param) ||
          (lhs.uses_second_param && rhs.uses_second_param)) {
        return std::nullopt;
      }
      return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                         lhs.uses_second_param || rhs.uses_second_param,
                         lhs.constant + rhs.constant};
    case c4c::backend::bir::BinaryOpcode::Sub:
      if (rhs.uses_first_param || rhs.uses_second_param) {
        return std::nullopt;
      }
      return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                         lhs.constant - rhs.constant};
    default:
      return std::nullopt;
  }
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }
  const auto& block = module.functions.front().blocks.front();
  if (!block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }
  return parse_i64(*block.terminator.value);
}

std::optional<std::int64_t> parse_minimal_constant_return_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (!function.params.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> values;
  auto resolve_value = [&](const c4c::backend::bir::Value& value)
      -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(value); imm.has_value()) {
      return imm;
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named || value.name.empty() ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    const auto it = values.find(value.name);
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  const auto& block = function.blocks.front();
  for (const auto& inst : block.insts) {
    const auto* bin = get_binary_inst(inst);
    if (bin == nullptr || bin->result.kind != c4c::backend::bir::Value::Kind::Named ||
        bin->result.name.empty() || bin->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto lhs = resolve_value(bin->lhs);
    const auto rhs = resolve_value(bin->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    std::optional<std::int64_t> result;
    switch (bin->opcode) {
      case c4c::backend::bir::BinaryOpcode::Add:
        result = *lhs + *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::Sub:
        result = *lhs - *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::Mul:
        result = *lhs * *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::SDiv:
        if (*rhs == 0) return std::nullopt;
        result = *lhs / *rhs;
        break;
      case c4c::backend::bir::BinaryOpcode::SRem:
        if (*rhs == 0) return std::nullopt;
        result = *lhs % *rhs;
        break;
      default:
        return std::nullopt;
    }

    values[bin->result.name] = *result;
  }

  if (!block.terminator.value.has_value()) {
    return std::nullopt;
  }
  return resolve_value(*block.terminator.value);
}

std::optional<MinimalTwoFunctionImmediateReturnSlice> parse_minimal_two_function_immediate_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.is_declaration || entry.is_declaration ||
      helper.return_type != TypeKind::I32 || entry.return_type != TypeKind::I32 ||
      !helper.params.empty() || !entry.params.empty() || helper.blocks.size() != 1 ||
      entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& helper_block = helper.blocks.front();
  const auto& entry_block = entry.blocks.front();
  if (helper_block.label != "entry" || entry_block.label != "entry" ||
      !helper_block.insts.empty() || !entry_block.insts.empty() ||
      !helper_block.terminator.value.has_value() || !entry_block.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto helper_imm = parse_i64(*helper_block.terminator.value);
  const auto entry_imm = parse_i64(*entry_block.terminator.value);
  if (!helper_imm.has_value() || !entry_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalTwoFunctionImmediateReturnSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .helper_imm = *helper_imm,
      .entry_imm = *entry_imm,
  };
}

std::optional<MinimalThreeFunctionImmediateReturnSlice> parse_minimal_three_function_immediate_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 3 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& first = module.functions[0];
  const auto& second = module.functions[1];
  const auto& entry = module.functions[2];
  const auto parse_function_imm = [](const Function& function) -> std::optional<std::int64_t> {
    if (function.is_declaration || function.return_type != TypeKind::I32 ||
        !function.params.empty() || function.blocks.size() != 1) {
      return std::nullopt;
    }
    const auto& block = function.blocks.front();
    if (block.label != "entry" || !block.insts.empty() || !block.terminator.value.has_value()) {
      return std::nullopt;
    }
    return parse_i64(*block.terminator.value);
  };

  const auto first_imm = parse_function_imm(first);
  const auto second_imm = parse_function_imm(second);
  const auto entry_imm = parse_function_imm(entry);
  if (!first_imm.has_value() || !second_imm.has_value() || !entry_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalThreeFunctionImmediateReturnSlice{
      .first_name = first.name,
      .second_name = second.name,
      .entry_name = entry.name,
      .first_imm = *first_imm,
      .second_imm = *second_imm,
      .entry_imm = *entry_imm,
  };
}

std::optional<bool> evaluate_minimal_i32_cmp(std::string_view predicate,
                                             std::string_view lhs_text,
                                             std::string_view rhs_text) {
  const auto lhs_raw = parse_i64(lhs_text);
  const auto rhs_raw = parse_i64(rhs_text);
  if (!lhs_raw.has_value() || !rhs_raw.has_value()) {
    return std::nullopt;
  }
  if (*lhs_raw < std::numeric_limits<std::int32_t>::min() ||
      *lhs_raw > std::numeric_limits<std::int32_t>::max() ||
      *rhs_raw < std::numeric_limits<std::int32_t>::min() ||
      *rhs_raw > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  const auto lhs_i32 = static_cast<std::int32_t>(*lhs_raw);
  const auto rhs_i32 = static_cast<std::int32_t>(*rhs_raw);
  const auto lhs_u32 = static_cast<std::uint32_t>(lhs_i32);
  const auto rhs_u32 = static_cast<std::uint32_t>(rhs_i32);

  if (predicate == "eq") return lhs_i32 == rhs_i32;
  if (predicate == "ne") return lhs_i32 != rhs_i32;
  if (predicate == "slt") return lhs_i32 < rhs_i32;
  if (predicate == "sle") return lhs_i32 <= rhs_i32;
  if (predicate == "sgt") return lhs_i32 > rhs_i32;
  if (predicate == "sge") return lhs_i32 >= rhs_i32;
  if (predicate == "ult") return lhs_u32 < rhs_u32;
  if (predicate == "ule") return lhs_u32 <= rhs_u32;
  if (predicate == "ugt") return lhs_u32 > rhs_u32;
  if (predicate == "uge") return lhs_u32 >= rhs_u32;
  return std::nullopt;
}

std::optional<MinimalAffineReturnSlice> parse_minimal_affine_return_slice(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.params.size() > 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.name.empty()) {
      return std::nullopt;
    }
  }

  std::vector<std::string_view> param_names;
  param_names.reserve(function.params.size());
  for (const auto& param : function.params) {
    param_names.push_back(param.name);
  }

  const auto& block = function.blocks.front();
  std::unordered_map<std::string, AffineValue> values;
  for (const auto& inst : block.insts) {
    const auto* bin = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (bin == nullptr || bin->result.kind != c4c::backend::bir::Value::Kind::Named ||
        bin->result.type != c4c::backend::bir::TypeKind::I32 ||
        bin->result.name.empty()) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_operand(bin->lhs, param_names, values);
    const auto rhs = lower_affine_operand(bin->rhs, param_names, values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine_values(*lhs, *rhs, bin->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    values[bin->result.name] = *combined;
  }

  if (!block.terminator.value.has_value()) {
    return std::nullopt;
  }
  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      .function_name = function.name,
      .param_count = function.params.size(),
      .uses_first_param = lowered_return->uses_first_param,
      .uses_second_param = lowered_return->uses_second_param,
      .constant = lowered_return->constant,
  };
}

std::optional<MinimalLocalTempSlice> parse_minimal_local_temp_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.str().empty() || slot->type_str != "i32" ||
      !slot->count.str().empty() || slot->align != 4) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || ret == nullptr || ret->type_str != "i32" ||
      !ret->value_str.has_value() || entry.insts.size() != 2) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&entry.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[1]);
  if (store == nullptr || load == nullptr || store->type_str != "i32" || load->type_str != "i32" ||
      store->ptr.str() != slot->result.str() || load->ptr.str() != slot->result.str() ||
      load->result.str().empty() || *ret->value_str != load->result.str()) {
    return std::nullopt;
  }

  const auto stored_imm = parse_i64(store->val.str());
  if (!stored_imm.has_value() || *stored_imm < std::numeric_limits<std::int32_t>::min() ||
      *stored_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  return MinimalLocalTempSlice{
      .function_name = function.name,
      .stored_imm = *stored_imm,
  };
}

std::optional<MinimalConstantBranchReturnSlice> parse_minimal_constant_branch_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.blocks.size() != 3 || !function.params.empty() ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* branch = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label != "entry" || branch == nullptr ||
      entry.insts.empty() || entry.insts.size() > 3) {
    return std::nullopt;
  }

  const auto parse_return_imm = [](const LirBlock& block) -> std::optional<std::int64_t> {
    if (!block.insts.empty()) {
      return std::nullopt;
    }
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    if (ret == nullptr || ret->type_str != "i32" || !ret->value_str.has_value()) {
      return std::nullopt;
    }
    return parse_i64(*ret->value_str);
  };

  const LirBlock* then_block = nullptr;
  const LirBlock* else_block = nullptr;
  for (const auto& block : function.blocks) {
    if (block.label == branch->true_label) {
      then_block = &block;
    } else if (block.label == branch->false_label) {
      else_block = &block;
    }
  }
  if (then_block == nullptr || else_block == nullptr) {
    return std::nullopt;
  }

  const auto then_ret = parse_return_imm(*then_block);
  const auto else_ret = parse_return_imm(*else_block);
  if (!then_ret.has_value() || !else_ret.has_value()) {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts.front());
  if (cmp == nullptr || cmp->is_float || cmp->type_str != "i32") {
    return std::nullopt;
  }

  auto branch_taken = evaluate_minimal_i32_cmp(cmp->predicate, cmp->lhs, cmp->rhs);
  if (!branch_taken.has_value()) {
    return std::nullopt;
  }

  if (entry.insts.size() >= 2) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
    if (cast == nullptr || cast->kind != LirCastKind::ZExt ||
        cast->from_type != "i1" || cast->operand != cmp->result ||
        cast->to_type != "i32") {
      return std::nullopt;
    }
    if (entry.insts.size() == 2 && branch->cond_name != cast->result) {
      return std::nullopt;
    }
  } else if (branch->cond_name != cmp->result) {
    return std::nullopt;
  }

  if (entry.insts.size() == 3) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
    const auto* cmp_to_zero = std::get_if<LirCmpOp>(&entry.insts[2]);
    if (cast == nullptr || cmp_to_zero == nullptr || cmp_to_zero->is_float ||
        cmp_to_zero->type_str != "i32" || cmp_to_zero->lhs != cast->result ||
        cmp_to_zero->rhs != "0" || branch->cond_name != cmp_to_zero->result) {
      return std::nullopt;
    }
    if (cmp_to_zero->predicate == "eq") {
      *branch_taken = !*branch_taken;
    } else if (cmp_to_zero->predicate != "ne") {
      return std::nullopt;
    }
  }

  return MinimalConstantBranchReturnSlice{
      .function_name = function.name,
      .returned_imm = *branch_taken ? *then_ret : *else_ret,
  };
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* store =
      entry.insts.size() == 2 ? std::get_if<StoreGlobalInst>(&entry.insts.front()) : nullptr;
  const auto* load =
      entry.insts.size() == 2 ? std::get_if<LoadGlobalInst>(&entry.insts.back()) : nullptr;
  if (entry.label != "entry" || store == nullptr || load == nullptr ||
      store->global_name != global.name || store->byte_offset != 0 ||
      store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store->value.type != TypeKind::I32 ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 ||
      load->global_name != global.name || load->byte_offset != 0 ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .init_imm = global.initializer->immediate,
      .store_imm = store->value.immediate,
      .align_bytes = load->align_bytes > 0 ? load->align_bytes : 4,
  };
}

std::optional<MinimalGlobalStoreReturnAndEntryReturnSlice>
parse_minimal_global_store_return_and_entry_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 2 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  auto try_match = [&](const Function& helper,
                       const Function& entry)
      -> std::optional<MinimalGlobalStoreReturnAndEntryReturnSlice> {
    if (helper.is_declaration || entry.is_declaration || helper.return_type != TypeKind::I32 ||
        entry.return_type != TypeKind::I32 || !helper.params.empty() || !entry.params.empty() ||
        !helper.local_slots.empty() || !entry.local_slots.empty() ||
        helper.blocks.size() != 1 || entry.blocks.size() != 1) {
      return std::nullopt;
    }

    const auto& helper_entry = helper.blocks.front();
    const auto& entry_entry = entry.blocks.front();
    const auto* store = helper_entry.insts.size() == 1
                            ? std::get_if<StoreGlobalInst>(&helper_entry.insts.front())
                            : nullptr;
    if (helper_entry.label != "entry" || store == nullptr ||
        store->global_name != global.name || store->byte_offset != 0 ||
        store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        store->value.type != TypeKind::I32 ||
        helper_entry.terminator.kind != TerminatorKind::Return ||
        !helper_entry.terminator.value.has_value() ||
        helper_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
        helper_entry.terminator.value->type != TypeKind::I32 ||
        entry_entry.label != "entry" || !entry_entry.insts.empty() ||
        entry_entry.terminator.kind != TerminatorKind::Return ||
        !entry_entry.terminator.value.has_value() ||
        entry_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
        entry_entry.terminator.value->type != TypeKind::I32) {
      return std::nullopt;
    }

    return MinimalGlobalStoreReturnAndEntryReturnSlice{
        .global_name = global.name,
        .helper_name = helper.name,
        .entry_name = entry.name,
        .init_imm = global.initializer->immediate,
        .store_imm = store->value.immediate,
        .helper_imm = helper_entry.terminator.value->immediate,
        .entry_imm = entry_entry.terminator.value->immediate,
        .align_bytes = store->align_bytes > 0 ? store->align_bytes : 4,
        .zero_initializer = global.initializer->immediate == 0,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "L" + label;
  }
  return ".L." + label;
}

std::string escape_asm_string(std::string_view raw_bytes) {
  std::ostringstream out;
  for (unsigned char ch : raw_bytes) {
    switch (ch) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\t': out << "\\t"; break;
      default:
        if (std::isprint(ch) != 0) {
          out << static_cast<char>(ch);
        } else {
          constexpr char kHex[] = "0123456789ABCDEF";
          out << '\\' << kHex[(ch >> 4) & 0xF] << kHex[ch & 0xF];
        }
        break;
    }
  }
  return out.str();
}

std::string emit_function_prelude(std::string_view target_triple,
                                  std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string emit_minimal_return_imm_asm(const c4c::backend::bir::Module& module,
                                        std::int64_t imm) {
  std::ostringstream out;
  const auto symbol =
      asm_symbol_name(module.target_triple, module.functions.front().name);
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(module.target_triple, symbol)
      << "  mov eax, " << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_function_immediate_return_asm(
    std::string_view target_triple,
    const MinimalTwoFunctionImmediateReturnSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, " << slice.helper_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_three_function_immediate_return_asm(
    std::string_view target_triple,
    const MinimalThreeFunctionImmediateReturnSlice& slice) {
  const auto first_symbol = asm_symbol_name(target_triple, slice.first_name);
  const auto second_symbol = asm_symbol_name(target_triple, slice.second_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, first_symbol)
      << "  mov eax, " << slice.first_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, second_symbol)
      << "  mov eax, " << slice.second_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

bool target_uses_x86_64_sysv_regs(std::string_view target_triple) {
  return target_triple.find("x86_64") != std::string::npos ||
         target_triple.find("amd64") != std::string::npos;
}

std::string emit_minimal_affine_return_asm(std::string_view target_triple,
                                           const MinimalAffineReturnSlice& slice) {
  if (slice.constant < std::numeric_limits<std::int32_t>::min() ||
      slice.constant > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }
  if ((slice.uses_first_param || slice.uses_second_param) &&
      !target_uses_x86_64_sysv_regs(target_triple)) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol);

  if (!slice.uses_first_param && !slice.uses_second_param) {
    out << "  mov eax, " << slice.constant << "\n"
        << "  ret\n";
    return out.str();
  }

  if (slice.uses_first_param) {
    out << "  mov eax, edi\n";
    if (slice.uses_second_param) {
      out << "  add eax, esi\n";
    }
  } else if (slice.uses_second_param) {
    out << "  mov eax, esi\n";
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_local_temp_asm(std::string_view target_triple,
                                        const MinimalLocalTempSlice& slice) {
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  mov eax, " << slice.stored_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_constant_branch_return_asm(
    std::string_view target_triple,
    const MinimalConstantBranchReturnSlice& slice) {
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  mov eax, " << slice.returned_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_countdown_loop_asm(std::string_view target_triple,
                                            const MinimalCountdownLoopSlice& slice) {
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  mov eax, " << slice.initial_imm << "\n"
      << ".L" << slice.loop_label << ":\n"
      << "  cmp eax, 0\n"
      << "  je .L" << slice.exit_label << "\n"
      << "  jmp .L" << slice.body_label << "\n"
      << ".L" << slice.body_label << ":\n"
      << "  sub eax, 1\n"
      << "  jmp .L" << slice.loop_label << "\n"
      << ".L" << slice.exit_label << ":\n"
      << "  ret\n";
  return out.str();
}

std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool is_zero_init) {
  std::ostringstream out;
  out << (is_zero_init ? ".bss\n" : ".data\n")
      << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @object\n";
  }
  if (align_bytes > 1) {
    out << ".p2align " << (align_bytes == 2 ? 1 : 2) << "\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

[[noreturn]] void throw_x86_rewrite_in_progress() {
  throw std::invalid_argument(
      "x86 backend emitter rewrite in progress: move ownership into the translated sibling codegen translation units instead of adding emit.cpp-local matchers");
}

}  // namespace

std::optional<std::string> try_emit_module(const c4c::backend::bir::Module& module) {
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_imm_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_constant_return_imm(module); imm.has_value()) {
    return emit_minimal_return_imm_asm(module, *imm);
  }
  if (const auto slice = parse_minimal_two_function_immediate_return_slice(module);
      slice.has_value()) {
    return emit_minimal_two_function_immediate_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_three_function_immediate_return_slice(module);
      slice.has_value()) {
    return emit_minimal_three_function_immediate_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_affine_return_slice(module); slice.has_value()) {
    return emit_minimal_affine_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_countdown_loop_slice(module); slice.has_value()) {
    return emit_minimal_countdown_loop_asm(module.target_triple, *slice);
  }
  if (const auto asm_text = try_emit_minimal_scalar_global_load_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_extern_scalar_global_load_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
      slice.has_value()) {
    return emit_minimal_scalar_global_store_reload_slice_asm(module.target_triple,
                                                             slice->function_name,
                                                             slice->global_name,
                                                             slice->init_imm,
                                                             slice->store_imm,
                                                             slice->align_bytes);
  }
  if (const auto slice = parse_minimal_global_store_return_and_entry_return_slice(module);
      slice.has_value()) {
    return emit_minimal_global_store_return_and_entry_return_asm(module.target_triple, *slice);
  }
  if (const auto asm_text = try_emit_minimal_global_two_field_struct_store_sub_sub_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_repeated_printf_local_i32_calls_bir_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

std::string emit_module(const c4c::backend::bir::Module& module) {
  if (const auto asm_text = try_emit_module(module); asm_text.has_value()) {
    return c4c::backend::x86::codegen::peephole::peephole_optimize(*asm_text);
  }
  throw_unsupported_direct_bir_module();
}

std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_constant_branch_return_slice(module); slice.has_value()) {
    return emit_minimal_constant_branch_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_local_temp_slice(module); slice.has_value()) {
    return emit_minimal_local_temp_asm(module.target_triple, *slice);
  }
  if (const auto asm_text = try_emit_minimal_void_helper_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_void_return_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_void_extern_call_return_imm_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_param_slot_add_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_extern_zero_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_arg_call_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_helper_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_second_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_second_local_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_first_local_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_first_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_second_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_double_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_variadic_sum2_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_variadic_double_bytes_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_counted_printf_ternary_loop_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_buffer_string_copy_printf_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_repeated_printf_immediates_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_string_literal_char_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  if (const auto asm_text = try_emit_prepared_lir_module(module); asm_text.has_value()) {
    return c4c::backend::x86::codegen::peephole::peephole_optimize(*asm_text);
  }
  throw_x86_rewrite_in_progress();
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(assembler::AssembleRequest{
      .asm_text = emit_module(module),
      .output_path = output_path,
  });
}

}  // namespace c4c::backend::x86
