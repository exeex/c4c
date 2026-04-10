#include "x86_codegen.hpp"

#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <limits>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

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

std::string direct_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
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

}  // namespace

std::string emit_minimal_local_temp_asm(std::string_view target_triple,
                                        std::string_view function_name,
                                        std::int64_t stored_imm) {
  const auto symbol = direct_symbol_name(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  mov eax, " << stored_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::optional<std::string> try_emit_minimal_local_temp_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_local_temp_slice(module); slice.has_value()) {
    return emit_minimal_local_temp_asm(module.target_triple, slice->function_name,
                                       slice->stored_imm);
  }
  return std::nullopt;
}

std::string emit_minimal_constant_branch_return_asm(std::string_view target_triple,
                                                    std::string_view function_name,
                                                    std::int64_t returned_imm) {
  const auto symbol = direct_symbol_name(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  mov eax, " << returned_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::optional<std::string> try_emit_minimal_constant_branch_return_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_constant_branch_return_slice(module); slice.has_value()) {
    return emit_minimal_constant_branch_return_asm(module.target_triple, slice->function_name,
                                                   slice->returned_imm);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
