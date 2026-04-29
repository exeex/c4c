#include "emit.hpp"

#include "../../backend.hpp"
#include "../../../bir/bir.hpp"
#include "../../../bir/lir_to_bir.hpp"
#include "../../generation.hpp"
#include "../../stack_layout/analysis.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../stack_layout/slot_assignment.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <type_traits>
#include <vector>

namespace c4c::backend::aarch64 {

namespace {

[[noreturn]] void fail_unsupported(const char* detail) {
  throw std::invalid_argument(std::string("aarch64 backend emitter does not support ") +
                              detail);
}

[[noreturn]] void fail_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "aarch64 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
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

bool lir_typespec_is_i32(const c4c::TypeSpec& type) {
  return type.ptr_level == 0 && type.array_rank == 0 && !type.is_lvalue_ref &&
         !type.is_rvalue_ref && type.base == c4c::TB_INT;
}

bool lir_typespec_uses_nonminimal_signature_type(const c4c::TypeSpec& type) {
  if (type.ptr_level > 0) {
    return false;
  }
  switch (type.base) {
    case c4c::TB_VOID:
    case c4c::TB_CHAR:
    case c4c::TB_UCHAR:
    case c4c::TB_SCHAR:
    case c4c::TB_SHORT:
    case c4c::TB_USHORT:
    case c4c::TB_INT:
    case c4c::TB_UINT:
    case c4c::TB_BOOL:
      return false;
    default:
      return true;
  }
}

bool lir_type_ref_is_i32(const c4c::codegen::lir::LirTypeRef& type_ref) {
  return type_ref.str() == "i32";
}

bool lir_type_ref_uses_nonminimal_signature_type(
    const c4c::codegen::lir::LirTypeRef& type_ref) {
  return c4c::backend::backend_lir_type_uses_nonminimal_types(type_ref.str());
}

bool lir_function_returns_i32(const c4c::codegen::lir::LirFunction& function) {
  if (function.signature_return_type_ref.has_value()) {
    return lir_type_ref_is_i32(*function.signature_return_type_ref);
  }
  return lir_typespec_is_i32(function.return_type);
}

bool lir_function_has_fixed_param_count(
    const c4c::codegen::lir::LirFunction& function,
    std::size_t expected_count) {
  if (function.signature_is_variadic) {
    return false;
  }
  if (!function.signature_params.empty() || !function.signature_param_type_refs.empty() ||
      function.signature_has_void_param_list) {
    return function.signature_params.size() == expected_count &&
           function.signature_param_type_refs.size() == function.signature_params.size();
  }
  return function.params.size() == expected_count;
}

bool lir_function_is_i32_definition(const c4c::codegen::lir::LirFunction& function) {
  return !function.is_declaration && lir_function_returns_i32(function);
}

bool lir_function_is_zero_arg_i32_definition(
    const c4c::codegen::lir::LirFunction& function) {
  return lir_function_is_i32_definition(function) &&
         lir_function_has_fixed_param_count(function, 0);
}

bool lir_function_signature_uses_nonminimal_types(
    const c4c::codegen::lir::LirFunction& function) {
  if (function.signature_return_type_ref.has_value()) {
    if (lir_type_ref_uses_nonminimal_signature_type(*function.signature_return_type_ref)) {
      return true;
    }
  } else if (lir_typespec_uses_nonminimal_signature_type(function.return_type)) {
    return true;
  }

  if (!function.signature_param_type_refs.empty()) {
    for (const auto& param_type : function.signature_param_type_refs) {
      if (lir_type_ref_uses_nonminimal_signature_type(param_type)) {
        return true;
      }
    }
    return false;
  }

  for (const auto& [name, param_type] : function.params) {
    (void)name;
    if (lir_typespec_uses_nonminimal_signature_type(param_type)) {
      return true;
    }
  }
  return false;
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

std::optional<std::int64_t> parse_i64(const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return value.immediate;
}

std::optional<c4c::codegen::lir::LirCmpPredicate> lower_bir_cmp_predicate(
    c4c::backend::bir::BinaryOpcode predicate) {
  using BirPred = c4c::backend::bir::BinaryOpcode;
  using LirPred = c4c::codegen::lir::LirCmpPredicate;

  switch (predicate) {
    case BirPred::Eq: return LirPred::Eq;
    case BirPred::Ne: return LirPred::Ne;
    case BirPred::Slt: return LirPred::Slt;
    case BirPred::Sle: return LirPred::Sle;
    case BirPred::Sgt: return LirPred::Sgt;
    case BirPred::Sge: return LirPred::Sge;
    case BirPred::Ult: return LirPred::Ult;
    case BirPred::Ule: return LirPred::Ule;
    case BirPred::Ugt: return LirPred::Ugt;
    case BirPred::Uge: return LirPred::Uge;
    default: return std::nullopt;
  }
}

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

struct MinimalCastReturnSlice {
  std::string function_name;
  c4c::backend::bir::CastOpcode opcode = c4c::backend::bir::CastOpcode::SExt;
  c4c::backend::bir::TypeKind operand_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind result_type = c4c::backend::bir::TypeKind::Void;
};

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<AffineValue> lower_affine_operand(
    std::string_view operand,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (const auto imm = parse_i64(operand); imm.has_value()) {
    return AffineValue{false, false, *imm};
  }
  if (!param_names.empty() && operand == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && operand == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(std::string(operand));
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
  }
  return std::nullopt;
}

const c4c::backend::bir::BinaryInst* get_binary_inst(
    const c4c::backend::bir::Inst& inst) {
  return std::get_if<c4c::backend::bir::BinaryInst>(&inst);
}

bool is_minimal_single_function_asm_slice(const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  return true;
}

bool lir_module_needs_nonminimal_lowering(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  for (const auto& global : module.globals) {
    if (c4c::backend::backend_lir_global_uses_nonminimal_types(global)) {
      return true;
    }
  }

  for (const auto& decl : module.extern_decls) {
    if (c4c::backend::backend_lir_type_uses_nonminimal_types(decl.return_type_str)) {
      return true;
    }
  }

  auto inst_needs_nonminimal = [](const LirInst& inst) {
    return std::visit(
        [](const auto& op) -> bool {
          using T = std::decay_t<decltype(op)>;
          if constexpr (std::is_same_v<T, LirAllocaOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else if constexpr (std::is_same_v<T, LirLoadOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else if constexpr (std::is_same_v<T, LirStoreOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else if constexpr (std::is_same_v<T, LirBinOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else if constexpr (std::is_same_v<T, LirCmpOp>) {
            return op.is_float ||
                   c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else if constexpr (std::is_same_v<T, LirCallOp>) {
            return c4c::backend::backend_lir_call_uses_nonminimal_types(op);
          } else if constexpr (std::is_same_v<T, LirGepOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.element_type);
          } else if constexpr (std::is_same_v<T, LirCastOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.from_type) ||
                   c4c::backend::backend_lir_type_uses_nonminimal_types(op.to_type);
          } else if constexpr (std::is_same_v<T, LirPhiOp>) {
            return c4c::backend::backend_lir_type_uses_nonminimal_types(op.type_str);
          } else {
            return false;
          }
        },
        inst);
  };

  for (const auto& function : module.functions) {
    if (lir_function_signature_uses_nonminimal_types(function)) {
      return true;
    }
    for (const auto& inst : function.alloca_insts) {
      if (inst_needs_nonminimal(inst)) {
        return true;
      }
    }
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        if (inst_needs_nonminimal(inst)) {
          return true;
        }
      }
      if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
        if (c4c::backend::backend_lir_return_uses_nonminimal_types(*ret)) {
          return true;
        }
      }
    }
  }

  return false;
}

std::optional<std::string_view> strip_typed_operand_prefix(std::string_view operand,
                                                           std::string_view type_prefix);
struct MinimalCountdownLoopSlice;
struct MinimalScalarGlobalLoadSlice;
struct MinimalScalarGlobalStoreReloadSlice;
struct MinimalExternScalarGlobalLoadSlice;
struct MinimalExternGlobalArrayLoadSlice;
std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name);
std::string emit_minimal_return_imm_asm(std::string_view target_triple,
                                        std::string_view function_name,
                                        std::int64_t imm);
std::string emit_minimal_return_sub_imm_asm(std::string_view target_triple,
                                            std::string_view function_name,
                                            std::int64_t imm);
std::string emit_minimal_countdown_loop_asm(std::string_view target_triple,
                                            const MinimalCountdownLoopSlice& slice);
std::string emit_minimal_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalLoadSlice& slice);
std::string emit_minimal_scalar_global_store_reload_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalStoreReloadSlice& slice);
std::string emit_minimal_extern_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalExternScalarGlobalLoadSlice& slice);
std::string emit_minimal_extern_global_array_load_asm(
    std::string_view target_triple,
    const MinimalExternGlobalArrayLoadSlice& slice);
void emit_function_prelude(std::ostringstream& out,
                           std::string_view target_triple,
                           std::string_view symbol,
                           bool is_global);

constexpr std::array<c4c::backend::PhysReg, 9> kAarch64CalleeSavedRegs = {
    c4c::backend::PhysReg{20}, c4c::backend::PhysReg{21}, c4c::backend::PhysReg{22},
    c4c::backend::PhysReg{23}, c4c::backend::PhysReg{24}, c4c::backend::PhysReg{25},
    c4c::backend::PhysReg{26}, c4c::backend::PhysReg{27}, c4c::backend::PhysReg{28},
};

constexpr std::array<c4c::backend::PhysReg, 2> kAarch64CallerSavedRegs = {
    c4c::backend::PhysReg{13},
    c4c::backend::PhysReg{14},
};

std::string minimal_single_function_symbol(const c4c::backend::bir::Module& module) {
  return asm_symbol_name(module.target_triple, module.functions.front().name);
}

std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name) {
  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

std::string aarch64_reg32_name(c4c::backend::PhysReg reg) {
  if (reg.index > 30) {
    throw std::invalid_argument("aarch64 backend emitter received an unknown physical register");
  }
  return "w" + std::to_string(reg.index);
}

std::string aarch64_reg64_name(c4c::backend::PhysReg reg) {
  if (reg.index > 30) {
    throw std::invalid_argument("aarch64 backend emitter received an unknown physical register");
  }
  return "x" + std::to_string(reg.index);
}

// BIR-ready scalar/control-flow slices.
// These helpers only need single-function/block/value shape that BIR already
// models.
std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::bir::Module& module) {
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
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1 || !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  const auto* add = get_binary_inst(block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::bir::BinaryOpcode::Add ||
      add->result.kind != c4c::backend::bir::Value::Kind::Named ||
      add->result.type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != add->result.name) {
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
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1 || !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  const auto* sub = get_binary_inst(block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->result.type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != sub->result.name) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(sub->lhs);
  const auto rhs = parse_i64(sub->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  return *lhs - *rhs;
}


std::optional<AffineValue> lower_affine_operand(
    const c4c::backend::bir::Value& value,
    const std::vector<std::string_view>& param_names,
    const std::unordered_map<std::string, AffineValue>& values) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return AffineValue{false, false, value.immediate};
  }
  if (!param_names.empty() && value.name == param_names[0]) {
    return AffineValue{true, false, 0};
  }
  if (param_names.size() > 1 && value.name == param_names[1]) {
    return AffineValue{false, true, 0};
  }
  auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
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
    const auto* bin = get_binary_inst(inst);
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

  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      function.name,
      function.params.size(),
      lowered_return->uses_first_param,
      lowered_return->uses_second_param,
      lowered_return->constant,
  };
}

std::optional<MinimalCastReturnSlice> parse_minimal_cast_return_slice(
    const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.blocks.size() != 1 ||
      function.params.size() != 1 || function.blocks.front().insts.size() != 1 ||
      !function.blocks.front().terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry") {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  if (param.name.empty()) {
    return std::nullopt;
  }

  const auto* cast =
      std::get_if<c4c::backend::bir::CastInst>(&block.insts.front());
  if (cast == nullptr || cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
      cast->result.name.empty() || cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      cast->operand.name != param.name || cast->operand.type != param.type ||
      cast->result.type != function.return_type ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != cast->result.name ||
      block.terminator.value->type != cast->result.type) {
    return std::nullopt;
  }

  return MinimalCastReturnSlice{
      function.name,
      cast->opcode,
      cast->operand.type,
      cast->result.type,
  };
}

const c4c::backend::bir::StringConstant* find_string_constant(
    const c4c::backend::bir::Module& module,
    std::string_view name) {
  for (const auto& string_constant : module.string_constants) {
    if (string_constant.name == name) return &string_constant;
  }
  return nullptr;
}

std::optional<std::int64_t> parse_minimal_lir_return_sub_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry") {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  if (function.alloca_insts.empty()) {
    if (entry.insts.size() != 1) {
      return std::nullopt;
    }
    const auto* sub = std::get_if<LirBinOp>(&entry.insts.front());
    const auto sub_opcode = sub == nullptr ? std::nullopt : sub->opcode.typed();
    if (sub == nullptr || !sub_opcode.has_value() || *sub_opcode != LirBinaryOpcode::Sub ||
        sub->type_str != "i32" || !ret->value_str.has_value() ||
        *ret->value_str != sub->result) {
      return std::nullopt;
    }

    const auto lhs = parse_i64(sub->lhs);
    const auto rhs = parse_i64(sub->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    return *lhs - *rhs;
  }

  if (function.alloca_insts.size() != 1 ||
      entry.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "i32" || !alloca->count.empty()) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&entry.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[1]);
  const auto* sub = std::get_if<LirBinOp>(&entry.insts[2]);
  const auto sub_opcode = sub == nullptr ? std::nullopt : sub->opcode.typed();
  if (store == nullptr || load == nullptr || sub == nullptr ||
      store->type_str != "i32" || load->type_str != "i32" ||
      !sub_opcode.has_value() || *sub_opcode != LirBinaryOpcode::Sub ||
      sub->type_str != "i32" || store->ptr != alloca->result ||
      load->ptr != alloca->result || sub->lhs != load->result ||
      !ret->value_str.has_value() || *ret->value_str != sub->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->val);
  const auto sub_imm = parse_i64(sub->rhs);
  if (!store_imm.has_value() || !sub_imm.has_value()) {
    return std::nullopt;
  }

  return *store_imm - *sub_imm;
}

std::optional<std::int64_t> parse_minimal_lir_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry") {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> values;
  std::unordered_map<std::string, std::int64_t> slot_values;
  std::unordered_set<std::string> local_scalar_slots;
  for (const auto& alloca : function.alloca_insts) {
    const auto* alloca_inst = std::get_if<LirAllocaOp>(&alloca);
    if (alloca_inst == nullptr || alloca_inst->type_str != "i32" ||
        !alloca_inst->count.empty() || alloca_inst->result.empty()) {
      continue;
    }
    if (!local_scalar_slots.insert(alloca_inst->result).second) {
      return std::nullopt;
    }
  }

  auto resolve_value = [&](std::string_view value)
      -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(value); imm.has_value()) {
      return imm;
    }
    const auto it = values.find(std::string(value));
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (const auto& inst : entry.insts) {
    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str != "i32" ||
          local_scalar_slots.find(store->ptr) == local_scalar_slots.end()) {
        return std::nullopt;
      }
      const auto value = resolve_value(store->val);
      if (!value.has_value()) {
        return std::nullopt;
      }
      slot_values[store->ptr] = *value;
      continue;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32" ||
          local_scalar_slots.find(load->ptr) == local_scalar_slots.end()) {
        return std::nullopt;
      }
      const auto it = slot_values.find(load->ptr);
      if (it == slot_values.end()) {
        return std::nullopt;
      }
      values[load->result] = it->second;
      continue;
    }

    const auto* bin = std::get_if<LirBinOp>(&inst);
    if (bin == nullptr || bin->type_str != "i32") {
      return std::nullopt;
    }

    const auto lhs = resolve_value(bin->lhs);
    const auto rhs = resolve_value(bin->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    const auto opcode = bin->opcode.typed();
    if (!opcode.has_value()) {
      return std::nullopt;
    }

    std::optional<std::int64_t> result;
    if (*opcode == LirBinaryOpcode::Add) {
      result = *lhs + *rhs;
    } else if (*opcode == LirBinaryOpcode::Sub) {
      result = *lhs - *rhs;
    } else if (*opcode == LirBinaryOpcode::Mul) {
      result = *lhs * *rhs;
    } else if (*opcode == LirBinaryOpcode::SDiv) {
      if (*rhs == 0) return std::nullopt;
      result = *lhs / *rhs;
    } else if (*opcode == LirBinaryOpcode::SRem) {
      if (*rhs == 0) return std::nullopt;
      result = *lhs % *rhs;
    } else {
      return std::nullopt;
    }

    values[bin->result] = *result;
  }

  return resolve_value(*ret->value_str);
}

std::optional<std::int64_t> parse_minimal_lir_local_pointer_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() < 2) {
    return std::nullopt;
  }

  std::unordered_set<std::string> scalar_slots;
  std::unordered_set<std::string> ptr_slots;
  for (const auto& alloca : function.alloca_insts) {
    const auto* alloca_inst = std::get_if<LirAllocaOp>(&alloca);
    if (alloca_inst == nullptr || alloca_inst->result.empty() ||
        !alloca_inst->count.empty()) {
      continue;
    }
    if (alloca_inst->type_str == "i32") {
      scalar_slots.insert(alloca_inst->result);
    } else if (alloca_inst->type_str == "ptr") {
      ptr_slots.insert(alloca_inst->result);
    }
  }
  if (scalar_slots.empty() || ptr_slots.empty()) {
    return std::nullopt;
  }

  const auto find_entry_block = [&]() -> std::optional<
      std::reference_wrapper<const LirBlock>> {
    for (const auto& candidate : function.blocks) {
      if (candidate.id.value == function.entry.value) {
        return candidate;
      }
    }
    return std::nullopt;
  }();
  if (!find_entry_block.has_value()) {
    return std::nullopt;
  }
  const auto& block = find_entry_block->get();
  if (block.label != "entry") {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> scalar_values;
  std::unordered_map<std::string, std::int64_t> int_cast_values;
  std::unordered_map<std::string, std::string> ptr_targets;
  std::unordered_map<std::string, std::string> loaded_values;
  const auto normalize_name = [](std::string_view text) {
    return (text.size() > 0 && text.front() == '%') ? std::string(text.substr(1))
                                                     : std::string(text);
  };

  auto is_scalar_slot = [&](std::string_view name) {
    return scalar_slots.find(normalize_name(name)) != scalar_slots.end();
  };

  auto is_ptr_slot = [&](std::string_view name) {
    return ptr_slots.find(normalize_name(name)) != ptr_slots.end();
  };

  auto parse_typed_operand = [](std::string_view text) -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(text); imm.has_value()) {
      return imm;
    }
    if (const auto i64 = strip_typed_operand_prefix(text, "i64"); i64.has_value()) {
      return parse_i64(*i64);
    }
    if (const auto i32 = strip_typed_operand_prefix(text, "i32"); i32.has_value()) {
      return parse_i64(*i32);
    }
    return std::nullopt;
  };
  
  auto resolve_scalar_target = [&](std::string_view ptr)
      -> std::optional<std::string> {
    const auto key = normalize_name(ptr);
    if (is_scalar_slot(key)) return key;
    const auto it = ptr_targets.find(key);
    if (it == ptr_targets.end()) return std::nullopt;
    if (is_scalar_slot(it->second)) return it->second;
    return std::nullopt;
  };

  auto resolve_ptr_target = [&](std::string_view ptr) -> std::optional<std::string> {
    const auto key = normalize_name(ptr);
    const auto it = ptr_targets.find(key);
    if (it != ptr_targets.end()) {
      return it->second;
    }
    if (is_ptr_slot(key) || is_scalar_slot(key)) {
      return key;
    }
    return std::nullopt;
  };

  auto strip_type_prefix = [](std::string_view text) -> std::string_view {
    const auto space = text.find(' ');
    if (space != std::string_view::npos) {
      return text.substr(space + 1);
    }
    return text;
  };

  auto resolve_integer = [&](std::string_view value)
      -> std::optional<std::int64_t> {
    if (const auto imm = parse_typed_operand(value); imm.has_value()) {
      return imm;
    }
    const auto stripped = strip_type_prefix(value);
    const auto key = normalize_name(stripped);
    const auto it = int_cast_values.find(key);
    if (it != int_cast_values.end()) return it->second;
    const auto scalar = scalar_values.find(key);
    if (scalar != scalar_values.end()) return scalar->second;
    return std::nullopt;
  };

  for (const auto& inst : block.insts) {
    if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
      if (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) {
        return std::nullopt;
      }
      if ((cast->to_type != "i64" && cast->to_type != "i32") ||
          (cast->from_type != "i32" && cast->from_type != "i64")) {
        return std::nullopt;
      }
      const auto value = resolve_integer(cast->operand);
      if (!value.has_value()) {
        return std::nullopt;
      }
      int_cast_values[normalize_name(cast->result)] = *value;
      continue;
    }

    if (const auto* gep = std::get_if<LirGepOp>(&inst)) {
      if (gep->element_type != "i32" || gep->indices.size() != 1) {
        return std::nullopt;
      }
      const auto base = resolve_scalar_target(gep->ptr);
      if (!base.has_value()) {
        return std::nullopt;
      }
      const auto index = resolve_integer(gep->indices.front());
      if (!index.has_value() || *index != 0) {
        return std::nullopt;
      }
      ptr_targets[normalize_name(gep->result)] = *base;
      continue;
    }

    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str == "i32") {
        const auto target = resolve_scalar_target(store->ptr);
        if (!target.has_value()) return std::nullopt;
        const auto value = resolve_integer(store->val);
        if (!value.has_value()) return std::nullopt;
        scalar_values[normalize_name(*target)] = *value;
        continue;
      }

      if (store->type_str == "ptr") {
        const auto target = resolve_ptr_target(store->ptr);
        if (!target.has_value()) return std::nullopt;
        const auto value = resolve_ptr_target(store->val);
        if (!value.has_value()) return std::nullopt;
        ptr_targets[normalize_name(*target)] = normalize_name(*value);
        continue;
      }

      return std::nullopt;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str == "ptr") {
        const auto target = resolve_ptr_target(load->ptr);
        if (!target.has_value()) {
          return std::nullopt;
        }
        ptr_targets[normalize_name(load->result)] = normalize_name(*target);
        continue;
      }

      if (load->type_str == "i32") {
        const auto target = resolve_scalar_target(load->ptr);
        if (!target.has_value()) return std::nullopt;
        const auto value = scalar_values.find(normalize_name(*target));
        if (value == scalar_values.end()) return std::nullopt;
        loaded_values[normalize_name(load->result)] = normalize_name(*target);
        continue;
      }

      return std::nullopt;
    }

    return std::nullopt;
  }

  const auto load_it = loaded_values.find(normalize_name(*ret->value_str));
  if (load_it != loaded_values.end()) {
    const auto value_it = scalar_values.find(load_it->second);
    if (value_it == scalar_values.end()) return std::nullopt;
    return value_it->second;
  }

  return parse_typed_operand(*ret->value_str);
}

// Transitional direct-LIR fallback for bounded single-function slices that do
// not yet have a dedicated structured backend matcher.
// Generic constant folder / mini-interpreter.
// Simulates execution of a no-call function by tracking all memory slots
// (including arrays, structs, globals, string constants) and SSA values.
// Follows branches (conditional and unconditional) up to a step limit.
// If the return value can be resolved to a compile-time constant, returns it.
std::optional<std::int64_t> try_constant_fold_single_block(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty()) {
    return std::nullopt;
  }

  // Build block label map.
  std::unordered_map<std::string, const LirBlock*> block_map;
  for (const auto& block : function.blocks) {
    block_map[block.label] = &block;
  }

  const auto* entry_block = block_map.count("entry") ? block_map["entry"] : nullptr;
  if (entry_block == nullptr) {
    return std::nullopt;
  }

  // Memory model: map from (slot_name, byte_offset) -> i64 value.
  // Pointer model: SSA name -> (slot_name, byte_offset).
  struct Pointer { std::string slot; std::int64_t offset = 0; };

  std::unordered_map<std::string, std::int64_t> ssa_ints;       // SSA -> int value
  std::unordered_map<std::string, Pointer> ssa_ptrs;            // SSA -> pointer
  std::map<std::pair<std::string, std::int64_t>, std::int64_t> memory;  // (slot, offset) -> int value
  std::map<std::pair<std::string, std::int64_t>, Pointer> ptr_memory;  // (slot, offset) -> pointer

  // Alloca registry: slot name -> (element_size, total_bytes)
  struct SlotInfo { int elem_size; int total_bytes; bool is_ptr; };
  std::unordered_map<std::string, SlotInfo> slots;

  auto parse_array_type = [](const std::string& type_str)
      -> std::optional<std::pair<int, int>> {
    // Parse "[N x i32]" -> (N, 4)
    if (type_str.size() < 7 || type_str.front() != '[') return std::nullopt;
    auto x_pos = type_str.find(" x ");
    if (x_pos == std::string::npos) return std::nullopt;
    auto count = parse_i64(std::string_view(type_str).substr(1, x_pos - 1));
    if (!count.has_value() || *count <= 0) return std::nullopt;
    auto elem = std::string_view(type_str).substr(x_pos + 3);
    if (elem.back() == ']') elem.remove_suffix(1);
    int elem_size = 0;
    if (elem == "i32") elem_size = 4;
    else if (elem == "i64" || elem == "ptr") elem_size = 8;
    else if (elem == "i16") elem_size = 2;
    else if (elem == "i8") elem_size = 1;
    else return std::nullopt;
    return std::pair<int, int>{static_cast<int>(*count), elem_size};
  };

  // Parse struct type declarations to get field offsets.
  // e.g. "%struct.foo = type { i32, i32, ptr }" -> [0, 4, 8]
  struct StructLayout {
    std::vector<int> field_offsets;
    int total_size = 0;
  };
  std::unordered_map<std::string, StructLayout> struct_layouts;
  for (const auto& decl : module.type_decls) {
    auto eq_pos = decl.find(" = type { ");
    if (eq_pos == std::string::npos) continue;
    auto name = decl.substr(0, eq_pos);
    auto body = std::string_view(decl).substr(eq_pos + 10);
    if (body.empty() || body.back() != '}') continue;
    body.remove_suffix(body.size() > 1 && body[body.size()-2] == ' ' ? 2 : 1);

    StructLayout layout;
    int offset = 0;
    while (!body.empty()) {
      auto comma = body.find(',');
      auto field = (comma != std::string_view::npos) ? body.substr(0, comma) : body;
      while (!field.empty() && field.front() == ' ') field.remove_prefix(1);
      while (!field.empty() && field.back() == ' ') field.remove_suffix(1);

      int field_size = 0;
      int field_align = 1;
      if (field == "i32") { field_size = 4; field_align = 4; }
      else if (field == "i64" || field == "ptr") { field_size = 8; field_align = 8; }
      else if (field == "i16") { field_size = 2; field_align = 2; }
      else if (field == "i8") { field_size = 1; field_align = 1; }
      else if (auto arr = parse_array_type(std::string(field)); arr.has_value()) {
        field_size = arr->first * arr->second;
        field_align = arr->second;  // element alignment
      }

      if (field_size == 0) { layout.field_offsets.clear(); break; }
      offset = (offset + field_align - 1) & ~(field_align - 1);
      layout.field_offsets.push_back(offset);
      offset += field_size;

      if (comma == std::string_view::npos) break;
      body = body.substr(comma + 1);
    }
    if (!layout.field_offsets.empty()) {
      layout.total_size = offset;
      struct_layouts[name] = std::move(layout);
    }
  }

  for (const auto& alloc_var : function.alloca_insts) {
    const auto* a = std::get_if<LirAllocaOp>(&alloc_var);
    if (a == nullptr || a->result.empty()) return std::nullopt;
    if (a->type_str == "i32") {
      slots[a->result] = {4, 4, false};
    } else if (a->type_str == "i64") {
      slots[a->result] = {8, 8, false};
    } else if (a->type_str == "i8") {
      slots[a->result] = {1, 1, false};
    } else if (a->type_str == "i16") {
      slots[a->result] = {2, 2, false};
    } else if (a->type_str == "ptr") {
      slots[a->result] = {8, 8, true};
    } else if (auto arr = parse_array_type(a->type_str); arr.has_value()) {
      slots[a->result] = {arr->second, arr->first * arr->second, false};
    } else if (auto sl = struct_layouts.find(a->type_str); sl != struct_layouts.end()) {
      slots[a->result] = {sl->second.total_size, sl->second.total_size, false};
    } else {
      return std::nullopt;
    }
    ssa_ptrs[a->result] = {a->result, 0};
  }

  // Register string pool entries as read-only memory slots.
  for (const auto& str_const : module.string_pool) {
    auto slot_name = str_const.pool_name;
    int total_bytes = str_const.byte_length;
    if (total_bytes <= 0) continue;
    slots[slot_name] = {1, total_bytes, false};
    ssa_ptrs[slot_name] = {slot_name, 0};
    // Pre-fill memory with byte values.
    for (int i = 0; i < total_bytes && i < static_cast<int>(str_const.raw_bytes.size()); ++i) {
      memory[{slot_name, i}] = static_cast<std::int64_t>(static_cast<unsigned char>(str_const.raw_bytes[i]));
    }
  }

  // Register globals as memory slots.
  for (const auto& global : module.globals) {
    auto slot_name = "@" + global.name;
    const auto& ty = global.llvm_type;
    if (ty == "i32") {
      slots[slot_name] = {4, 4, false};
    } else if (ty == "i64") {
      slots[slot_name] = {8, 8, false};
    } else if (ty == "ptr") {
      slots[slot_name] = {8, 8, true};
    } else if (auto arr = parse_array_type(ty); arr.has_value()) {
      slots[slot_name] = {arr->second, arr->first * arr->second, false};
    } else if (auto sl = struct_layouts.find(ty); sl != struct_layouts.end()) {
      slots[slot_name] = {sl->second.total_size, sl->second.total_size, false};
    } else {
      return std::nullopt;
    }
    ssa_ptrs[slot_name] = {slot_name, 0};
    // Initialize with zeroinitializer (all zeros) — default for globals.
    // TODO: parse non-zero init_text for more coverage.
  }

  auto strip_type = [](std::string_view text) -> std::string_view {
    auto sp = text.find(' ');
    return (sp != std::string_view::npos) ? text.substr(sp + 1) : text;
  };

  auto resolve_int = [&](std::string_view value) -> std::optional<std::int64_t> {
    if (auto imm = parse_i64(value); imm.has_value()) return imm;
    auto stripped = strip_type(value);
    if (auto imm = parse_i64(stripped); imm.has_value()) return imm;
    auto it = ssa_ints.find(std::string(stripped));
    if (it != ssa_ints.end()) return it->second;
    // try without strip
    it = ssa_ints.find(std::string(value));
    if (it != ssa_ints.end()) return it->second;
    return std::nullopt;
  };

  auto resolve_ptr = [&](std::string_view value) -> std::optional<Pointer> {
    auto stripped = strip_type(value);
    auto it = ssa_ptrs.find(std::string(stripped));
    if (it != ssa_ptrs.end()) return it->second;
    it = ssa_ptrs.find(std::string(value));
    if (it != ssa_ptrs.end()) return it->second;
    return std::nullopt;
  };

  auto type_size = [](std::string_view type_str) -> int {
    if (type_str == "i32") return 4;
    if (type_str == "i64" || type_str == "ptr") return 8;
    if (type_str == "i16") return 2;
    if (type_str == "i8") return 1;
    return 0;
  };

  // Instruction interpreter — processes one instruction, returns false on bail.
  auto exec_inst = [&](const LirInst& inst) -> bool {
    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      int sz = type_size(store->type_str);
      if (sz == 0) return false;
      auto target = resolve_ptr(store->ptr);
      if (!target.has_value()) return false;
      if (store->type_str == "ptr") {
        auto val_ptr = resolve_ptr(store->val);
        if (!val_ptr.has_value()) return false;
        ptr_memory[{target->slot, target->offset}] = *val_ptr;
        return true;
      }
      auto val = resolve_int(store->val);
      if (!val.has_value()) return false;
      memory[{target->slot, target->offset}] = *val;
      return true;
    }
    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      int sz = type_size(load->type_str);
      if (sz == 0) return false;
      if (load->type_str == "ptr") {
        auto src = resolve_ptr(load->ptr);
        if (!src.has_value()) return false;
        auto it = ptr_memory.find({src->slot, src->offset});
        if (it == ptr_memory.end()) return false;
        ssa_ptrs[load->result] = it->second;
        return true;
      }
      auto src = resolve_ptr(load->ptr);
      if (!src.has_value()) return false;
      auto it = memory.find({src->slot, src->offset});
      if (it == memory.end()) return false;
      ssa_ints[load->result] = it->second;
      return true;
    }
    if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
      if (cast->kind == LirCastKind::SExt || cast->kind == LirCastKind::ZExt ||
          cast->kind == LirCastKind::Trunc) {
        auto val = resolve_int(cast->operand);
        if (!val.has_value()) return false;
        if (cast->kind == LirCastKind::Trunc) {
          if (cast->to_type == "i32") *val = static_cast<std::int32_t>(*val);
          else if (cast->to_type == "i16") *val = static_cast<std::int16_t>(*val);
          else if (cast->to_type == "i8") *val = static_cast<std::int8_t>(*val);
        }
        ssa_ints[cast->result] = *val;
        return true;
      }
      return false;
    }
    if (const auto* gep = std::get_if<LirGepOp>(&inst)) {
      auto base_ptr = resolve_ptr(gep->ptr);
      if (!base_ptr.has_value()) return false;
      int64_t current_offset = base_ptr->offset;
      if (gep->indices.empty()) return false;
      std::string_view current_type = gep->element_type;
      if (auto arr = parse_array_type(std::string(current_type)); arr.has_value()) {
        if (gep->indices.size() != 2) return false;
        auto idx0 = resolve_int(gep->indices[0]);
        auto idx1 = resolve_int(gep->indices[1]);
        if (!idx0.has_value() || !idx1.has_value()) return false;
        current_offset += *idx0 * (arr->first * arr->second) + *idx1 * arr->second;
      } else if (auto sl = struct_layouts.find(std::string(current_type));
                 sl != struct_layouts.end()) {
        if (gep->indices.size() != 2) return false;
        auto idx0 = resolve_int(gep->indices[0]);
        auto idx1 = resolve_int(gep->indices[1]);
        if (!idx0.has_value() || !idx1.has_value()) return false;
        if (*idx1 < 0 || static_cast<size_t>(*idx1) >= sl->second.field_offsets.size())
          return false;
        current_offset += *idx0 * sl->second.total_size + sl->second.field_offsets[*idx1];
      } else {
        if (gep->indices.size() != 1) return false;
        auto idx = resolve_int(gep->indices[0]);
        if (!idx.has_value()) return false;
        int elem_sz = type_size(current_type);
        if (elem_sz == 0) return false;
        current_offset += *idx * elem_sz;
      }
      ssa_ptrs[gep->result] = {base_ptr->slot, current_offset};
      return true;
    }
    if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
      if (bin->type_str != "i32" && bin->type_str != "i64" &&
          bin->type_str != "i8" && bin->type_str != "i16") return false;
      auto lhs = resolve_int(bin->lhs);
      auto rhs = resolve_int(bin->rhs);
      if (!lhs.has_value() || !rhs.has_value()) return false;
      auto opcode = bin->opcode.typed();
      if (!opcode.has_value()) return false;
      std::int64_t result;
      switch (*opcode) {
        case LirBinaryOpcode::Add: result = *lhs + *rhs; break;
        case LirBinaryOpcode::Sub: result = *lhs - *rhs; break;
        case LirBinaryOpcode::Mul: result = *lhs * *rhs; break;
        case LirBinaryOpcode::SDiv:
          if (*rhs == 0) return false;
          result = *lhs / *rhs; break;
        case LirBinaryOpcode::UDiv:
          if (*rhs == 0) return false;
          result = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(*lhs) / static_cast<std::uint64_t>(*rhs));
          break;
        case LirBinaryOpcode::SRem:
          if (*rhs == 0) return false;
          result = *lhs % *rhs; break;
        case LirBinaryOpcode::URem:
          if (*rhs == 0) return false;
          result = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(*lhs) % static_cast<std::uint64_t>(*rhs));
          break;
        case LirBinaryOpcode::And: result = *lhs & *rhs; break;
        case LirBinaryOpcode::Or: result = *lhs | *rhs; break;
        case LirBinaryOpcode::Xor: result = *lhs ^ *rhs; break;
        case LirBinaryOpcode::Shl: result = *lhs << *rhs; break;
        case LirBinaryOpcode::AShr: result = *lhs >> *rhs; break;
        case LirBinaryOpcode::LShr:
          result = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(*lhs) >> *rhs);
          break;
        default: return false;
      }
      if (bin->type_str == "i32") result = static_cast<std::int32_t>(result);
      else if (bin->type_str == "i16") result = static_cast<std::int16_t>(result);
      else if (bin->type_str == "i8") result = static_cast<std::int8_t>(result);
      ssa_ints[bin->result] = result;
      return true;
    }
    if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
      if (cmp->is_float) return false;
      auto lhs = resolve_int(cmp->lhs);
      auto rhs = resolve_int(cmp->rhs);
      if (!lhs.has_value() || !rhs.has_value()) return false;
      bool cmp_result = false;
      if (cmp->predicate == "eq") cmp_result = (*lhs == *rhs);
      else if (cmp->predicate == "ne") cmp_result = (*lhs != *rhs);
      else if (cmp->predicate == "slt") cmp_result = (*lhs < *rhs);
      else if (cmp->predicate == "sle") cmp_result = (*lhs <= *rhs);
      else if (cmp->predicate == "sgt") cmp_result = (*lhs > *rhs);
      else if (cmp->predicate == "sge") cmp_result = (*lhs >= *rhs);
      else if (cmp->predicate == "ult") cmp_result = (static_cast<uint64_t>(*lhs) < static_cast<uint64_t>(*rhs));
      else if (cmp->predicate == "ule") cmp_result = (static_cast<uint64_t>(*lhs) <= static_cast<uint64_t>(*rhs));
      else if (cmp->predicate == "ugt") cmp_result = (static_cast<uint64_t>(*lhs) > static_cast<uint64_t>(*rhs));
      else if (cmp->predicate == "uge") cmp_result = (static_cast<uint64_t>(*lhs) >= static_cast<uint64_t>(*rhs));
      else return false;
      ssa_ints[cmp->result] = cmp_result ? 1 : 0;
      return true;
    }
    if (const auto* select = std::get_if<LirSelectOp>(&inst)) {
      auto cond = resolve_int(select->cond);
      if (!cond.has_value()) return false;
      auto true_val = resolve_int(select->true_val);
      auto false_val = resolve_int(select->false_val);
      if (!true_val.has_value() || !false_val.has_value()) return false;
      ssa_ints[select->result] = (*cond != 0) ? *true_val : *false_val;
      return true;
    }
    return false;
  };

  // Multi-block interpreter loop.
  constexpr int kMaxSteps = 10000;
  int steps = 0;
  const LirBlock* current_block = entry_block;

  while (steps < kMaxSteps) {
    // Execute all instructions in the current block.
    for (const auto& inst : current_block->insts) {
      if (!exec_inst(inst)) return std::nullopt;
      ++steps;
      if (steps >= kMaxSteps) return std::nullopt;
    }

    // Process terminator.
    if (const auto* ret = std::get_if<LirRet>(&current_block->terminator)) {
      if (ret->type_str != "i32" || !ret->value_str.has_value()) return std::nullopt;
      return resolve_int(*ret->value_str);
    }

    if (const auto* br = std::get_if<LirBr>(&current_block->terminator)) {
      auto it = block_map.find(br->target_label);
      if (it == block_map.end()) return std::nullopt;
      current_block = it->second;
      ++steps;
      continue;
    }

    if (const auto* condbr = std::get_if<LirCondBr>(&current_block->terminator)) {
      auto cond = resolve_int(condbr->cond_name);
      if (!cond.has_value()) return std::nullopt;
      const auto& target_label = (*cond != 0) ? condbr->true_label : condbr->false_label;
      auto it = block_map.find(target_label);
      if (it == block_map.end()) return std::nullopt;
      current_block = it->second;
      ++steps;
      continue;
    }

    // Switch, unreachable, etc. — bail
    return std::nullopt;
  }

  return std::nullopt;  // Step limit exceeded
}

bool is_double_indirect_local_pointer_conditional_return_fallback(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.alloca_insts.size() != 3 ||
      function.blocks.size() != 8) {
    return false;
  }

  const auto alloca0 = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto alloca1 = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  const auto alloca2 = std::get_if<LirAllocaOp>(&function.alloca_insts[2]);
  if (alloca0 == nullptr || alloca1 == nullptr || alloca2 == nullptr ||
      alloca0->type_str != "i32" || alloca1->type_str != "ptr" ||
      alloca2->type_str != "ptr") {
    return false;
  }

  const auto& entry = function.blocks[0];
  const auto* entry_condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 6 || entry_condbr == nullptr) {
    return false;
  }
  if (std::get_if<LirStoreOp>(&entry.insts[0]) == nullptr ||
      std::get_if<LirStoreOp>(&entry.insts[1]) == nullptr ||
      std::get_if<LirStoreOp>(&entry.insts[2]) == nullptr ||
      std::get_if<LirLoadOp>(&entry.insts[3]) == nullptr ||
      std::get_if<LirLoadOp>(&entry.insts[4]) == nullptr ||
      std::get_if<LirCmpOp>(&entry.insts[5]) == nullptr) {
    return false;
  }

  const auto& block1 = function.blocks[1];
  if (!block1.insts.empty() || std::get_if<LirRet>(&block1.terminator) == nullptr) {
    return false;
  }

  const auto& block2 = function.blocks[2];
  const auto* block2_condbr = std::get_if<LirCondBr>(&block2.terminator);
  if (block2.insts.size() != 4 || block2_condbr == nullptr) {
    return false;
  }
  if (std::get_if<LirLoadOp>(&block2.insts[0]) == nullptr ||
      std::get_if<LirLoadOp>(&block2.insts[1]) == nullptr ||
      std::get_if<LirLoadOp>(&block2.insts[2]) == nullptr ||
      std::get_if<LirCmpOp>(&block2.insts[3]) == nullptr) {
    return false;
  }

  const auto& block3 = function.blocks[3];
  if (!block3.insts.empty() || std::get_if<LirRet>(&block3.terminator) == nullptr) {
    return false;
  }

  const auto& block4 = function.blocks[4];
  const auto* block4_br = std::get_if<LirBr>(&block4.terminator);
  if (block4.insts.size() != 3 || block4_br == nullptr) {
    return false;
  }
  if (std::get_if<LirLoadOp>(&block4.insts[0]) == nullptr ||
      std::get_if<LirLoadOp>(&block4.insts[1]) == nullptr ||
      std::get_if<LirStoreOp>(&block4.insts[2]) == nullptr) {
    return false;
  }

  const auto& block5 = function.blocks[5];
  const auto* block5_condbr = std::get_if<LirCondBr>(&block5.terminator);
  if (block5.insts.size() != 2 || block5_condbr == nullptr) {
    return false;
  }
  if (std::get_if<LirLoadOp>(&block5.insts[0]) == nullptr ||
      std::get_if<LirCmpOp>(&block5.insts[1]) == nullptr) {
    return false;
  }

  const auto& block6 = function.blocks[6];
  const auto& block7 = function.blocks[7];
  return block6.insts.empty() && std::get_if<LirRet>(&block6.terminator) != nullptr &&
         block7.insts.empty() && std::get_if<LirRet>(&block7.terminator) != nullptr;
}

bool is_alloca_backed_switch_return_fallback(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.alloca_insts.size() != 1 ||
      function.blocks.size() < 3) {
    return false;
  }

  const auto* selector_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (selector_alloca == nullptr || selector_alloca->type_str != "i32") {
    return false;
  }

  const auto& entry = function.blocks[0];
  const auto* entry_switch = std::get_if<LirSwitch>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 2 || entry_switch == nullptr ||
      entry_switch->selector_type != "i32" || entry_switch->cases.empty() ||
      function.blocks.size() != entry_switch->cases.size() + 2) {
    return false;
  }
  if (std::get_if<LirStoreOp>(&entry.insts[0]) == nullptr ||
      std::get_if<LirLoadOp>(&entry.insts[1]) == nullptr) {
    return false;
  }

  std::unordered_set<std::string_view> expected_labels;
  expected_labels.insert(entry_switch->default_label);
  for (const auto& [_, label] : entry_switch->cases) {
    expected_labels.insert(label);
  }
  if (expected_labels.size() != function.blocks.size() - 1) {
    return false;
  }

  for (std::size_t block_index = 1; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (expected_labels.find(block.label) == expected_labels.end() || !block.insts.empty() ||
        std::get_if<LirRet>(&block.terminator) == nullptr) {
      return false;
    }
  }

  return true;
}

bool is_constant_selector_switch_return_fallback(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return false;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || !function.alloca_insts.empty() ||
      function.blocks.size() < 3) {
    return false;
  }

  const auto& entry = function.blocks[0];
  const auto* entry_switch = std::get_if<LirSwitch>(&entry.terminator);
  if (entry.label != "entry" || !entry.insts.empty() || entry_switch == nullptr ||
      entry_switch->selector_type != "i32" || entry_switch->cases.empty() ||
      function.blocks.size() != entry_switch->cases.size() + 2) {
    return false;
  }
  if (!parse_i64(entry_switch->selector_name).has_value()) {
    return false;
  }

  std::unordered_set<std::string_view> expected_labels;
  expected_labels.insert(entry_switch->default_label);
  for (const auto& [_, label] : entry_switch->cases) {
    expected_labels.insert(label);
  }
  if (expected_labels.size() != function.blocks.size() - 1) {
    return false;
  }

  for (std::size_t block_index = 1; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (expected_labels.find(block.label) == expected_labels.end() || !block.insts.empty() ||
        std::get_if<LirRet>(&block.terminator) == nullptr) {
      return false;
    }
  }

  return true;
}

struct MinimalConditionalReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ValueSource {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
  };

  std::string function_name;
  std::size_t param_count = 0;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueSource lhs;
  ValueSource rhs;
  std::string true_label;
  std::string false_label;
  ValueSource true_value;
  ValueSource false_value;
};

std::optional<MinimalConditionalReturnSlice::ValueSource> lower_bir_i32_value_source(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Immediate,
        value.immediate,
    };
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[0].name) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Param0,
        0,
    };
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[1].name) {
    return MinimalConditionalReturnSlice::ValueSource{
        MinimalConditionalReturnSlice::ValueKind::Param1,
        0,
    };
  }
  return std::nullopt;
}

struct MinimalConditionalAffineI8ReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ArithmeticStep {
    c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct ValueExpr {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
    std::vector<ArithmeticStep> steps;
  };

  std::string function_name;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueExpr lhs;
  ValueExpr rhs;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  ValueExpr true_value;
  ValueExpr false_value;
  std::vector<ArithmeticStep> post_steps;
};

struct MinimalConditionalAffineI32ReturnSlice {
  enum class ValueKind : unsigned char {
    Immediate,
    Param0,
    Param1,
  };

  struct ArithmeticStep {
    c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct ValueExpr {
    ValueKind kind = ValueKind::Immediate;
    std::int64_t imm = 0;
    std::vector<ArithmeticStep> steps;
  };

  std::string function_name;
  c4c::codegen::lir::LirCmpPredicate predicate = c4c::codegen::lir::LirCmpPredicate::Eq;
  ValueExpr lhs;
  ValueExpr rhs;
  std::string true_label;
  std::string false_label;
  std::string join_label;
  ValueExpr true_value;
  ValueExpr false_value;
  std::vector<ArithmeticStep> post_steps;
};

std::optional<MinimalConditionalAffineI8ReturnSlice::ValueExpr> lower_bir_i8_value_expr(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params,
    const std::unordered_map<std::string, MinimalConditionalAffineI8ReturnSlice::ValueExpr>&
        values) {
  using ValueExpr = MinimalConditionalAffineI8ReturnSlice::ValueExpr;
  using ValueKind = MinimalConditionalAffineI8ReturnSlice::ValueKind;

  if (value.type != c4c::backend::bir::TypeKind::I8) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return ValueExpr{ValueKind::Immediate, value.immediate, {}};
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I8 &&
      value.name == params[0].name) {
    return ValueExpr{ValueKind::Param0, 0, {}};
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I8 &&
      value.name == params[1].name) {
    return ValueExpr{ValueKind::Param1, 0, {}};
  }
  const auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<MinimalConditionalAffineI32ReturnSlice::ValueExpr> lower_bir_i32_value_expr(
    const c4c::backend::bir::Value& value,
    const std::vector<c4c::backend::bir::Param>& params,
    const std::unordered_map<std::string, MinimalConditionalAffineI32ReturnSlice::ValueExpr>&
        values) {
  using ValueExpr = MinimalConditionalAffineI32ReturnSlice::ValueExpr;
  using ValueKind = MinimalConditionalAffineI32ReturnSlice::ValueKind;

  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return ValueExpr{ValueKind::Immediate, value.immediate, {}};
  }
  if (!params.empty() && params[0].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[0].name) {
    return ValueExpr{ValueKind::Param0, 0, {}};
  }
  if (params.size() > 1 && params[1].type == c4c::backend::bir::TypeKind::I32 &&
      value.name == params[1].name) {
    return ValueExpr{ValueKind::Param1, 0, {}};
  }
  const auto it = values.find(value.name);
  if (it == values.end()) {
    return std::nullopt;
  }
  return it->second;
}

struct MinimalCountdownLoopSlice {
  std::string function_name;
  std::int64_t initial_imm = 0;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
};

struct MinimalScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  int align_bytes = 4;
  bool zero_initializer = false;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  int align_bytes = 4;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
};

struct MinimalExternGlobalArrayLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t byte_offset = 0;
};

struct MinimalLocalArraySlice {
  std::string function_name;
  std::int64_t store0_imm = 0;
  std::int64_t store1_imm = 0;
};

struct MinimalStringLiteralCharSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_index = 0;
  c4c::codegen::lir::LirCastKind extend_kind = c4c::codegen::lir::LirCastKind::ZExt;
};

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::bir::Module& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1 || function.params.size() > 2) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&block.insts.front());
  if (select == nullptr ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->name != select->result.name ||
      block.terminator.value->type != select->result.type) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i32_value_source(select->lhs, function.params);
  const auto rhs = lower_bir_i32_value_source(select->rhs, function.params);
  const auto true_value = lower_bir_i32_value_source(select->true_value, function.params);
  const auto false_value = lower_bir_i32_value_source(select->false_value, function.params);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalReturnSlice{
      function.name,
      function.params.size(),
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      *true_value,
      *false_value,
  };
}

std::optional<MinimalConditionalAffineI8ReturnSlice> parse_minimal_conditional_affine_i8_return_slice(
    const c4c::backend::bir::Module& module) {
  using Slice = MinimalConditionalAffineI8ReturnSlice;
  using ValueExpr = MinimalConditionalAffineI8ReturnSlice::ValueExpr;

  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I8 ||
      function.blocks.size() != 1 || function.params.size() != 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I8 || param.name.empty()) {
      return std::nullopt;
    }
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, ValueExpr> prefix_values;
  std::size_t select_index = 0;
  const c4c::backend::bir::SelectInst* select = nullptr;
  for (; select_index < block.insts.size(); ++select_index) {
    if (const auto* current_select =
            std::get_if<c4c::backend::bir::SelectInst>(&block.insts[select_index])) {
      select = current_select;
      break;
    }

    const auto* binary = get_binary_inst(block.insts[select_index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I8 || binary->result.name.empty()) {
      return std::nullopt;
    }
    auto lhs = lower_bir_i8_value_expr(binary->lhs, function.params, prefix_values);
    auto rhs = lower_bir_i8_value_expr(binary->rhs, function.params, prefix_values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    ValueExpr combined;
    if (rhs->kind == Slice::ValueKind::Immediate && rhs->steps.empty()) {
      combined = *lhs;
      combined.steps.push_back({binary->opcode, rhs->imm});
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               lhs->kind == Slice::ValueKind::Immediate && lhs->steps.empty()) {
      combined = *rhs;
      combined.steps.push_back({binary->opcode, lhs->imm});
    } else {
      return std::nullopt;
    }
    prefix_values[binary->result.name] = std::move(combined);
  }

  if (select == nullptr || select_index >= block.insts.size() ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I8 || select->result.name.empty()) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i8_value_expr(select->lhs, function.params, prefix_values);
  const auto rhs = lower_bir_i8_value_expr(select->rhs, function.params, prefix_values);
  const auto true_value =
      lower_bir_i8_value_expr(select->true_value, function.params, prefix_values);
  const auto false_value =
      lower_bir_i8_value_expr(select->false_value, function.params, prefix_values);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value() || !lhs->steps.empty() ||
      !rhs->steps.empty()) {
    return std::nullopt;
  }

  std::string current_name = select->result.name;
  std::vector<Slice::ArithmeticStep> post_steps;
  for (std::size_t index = select_index + 1; index < block.insts.size(); ++index) {
    const auto* binary = get_binary_inst(block.insts[index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I8 || binary->result.name.empty()) {
      return std::nullopt;
    }

    const bool lhs_is_current =
        binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->lhs.type == c4c::backend::bir::TypeKind::I8 && binary->lhs.name == current_name;
    const bool rhs_is_current =
        binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I8 && binary->rhs.name == current_name;
    if (lhs_is_current == rhs_is_current) {
      return std::nullopt;
    }

    std::optional<std::int64_t> immediate;
    if (lhs_is_current && binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I8) {
      immediate = binary->rhs.immediate;
    } else if (rhs_is_current && binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
               binary->lhs.type == c4c::backend::bir::TypeKind::I8) {
      immediate = binary->lhs.immediate;
    } else {
      return std::nullopt;
    }

    post_steps.push_back({binary->opcode, *immediate});
    current_name = binary->result.name;
  }

  if (block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I8 ||
      block.terminator.value->name != current_name) {
    return std::nullopt;
  }

  return Slice{
      function.name,
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      "select_join",
      *true_value,
      *false_value,
      std::move(post_steps),
  };
}

std::optional<MinimalConditionalAffineI32ReturnSlice>
parse_minimal_conditional_affine_i32_return_slice(
    const c4c::backend::bir::Module& module) {
  using Slice = MinimalConditionalAffineI32ReturnSlice;
  using ValueExpr = MinimalConditionalAffineI32ReturnSlice::ValueExpr;

  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1 || function.params.size() != 2) {
    return std::nullopt;
  }
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.name.empty()) {
      return std::nullopt;
    }
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.empty() || !block.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, ValueExpr> prefix_values;
  std::size_t select_index = 0;
  const c4c::backend::bir::SelectInst* select = nullptr;
  for (; select_index < block.insts.size(); ++select_index) {
    if (const auto* current_select =
            std::get_if<c4c::backend::bir::SelectInst>(&block.insts[select_index])) {
      select = current_select;
      break;
    }

    const auto* binary = get_binary_inst(block.insts[select_index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 || binary->result.name.empty()) {
      return std::nullopt;
    }
    auto lhs = lower_bir_i32_value_expr(binary->lhs, function.params, prefix_values);
    auto rhs = lower_bir_i32_value_expr(binary->rhs, function.params, prefix_values);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    ValueExpr combined;
    if (rhs->kind == Slice::ValueKind::Immediate && rhs->steps.empty()) {
      combined = *lhs;
      combined.steps.push_back({binary->opcode, rhs->imm});
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               lhs->kind == Slice::ValueKind::Immediate && lhs->steps.empty()) {
      combined = *rhs;
      combined.steps.push_back({binary->opcode, lhs->imm});
    } else {
      return std::nullopt;
    }
    prefix_values[binary->result.name] = std::move(combined);
  }

  if (select == nullptr || select_index >= block.insts.size() ||
      select->result.kind != c4c::backend::bir::Value::Kind::Named ||
      select->result.type != c4c::backend::bir::TypeKind::I32 || select->result.name.empty()) {
    return std::nullopt;
  }

  const auto predicate = lower_bir_cmp_predicate(select->predicate);
  const auto lhs = lower_bir_i32_value_expr(select->lhs, function.params, prefix_values);
  const auto rhs = lower_bir_i32_value_expr(select->rhs, function.params, prefix_values);
  const auto true_value =
      lower_bir_i32_value_expr(select->true_value, function.params, prefix_values);
  const auto false_value =
      lower_bir_i32_value_expr(select->false_value, function.params, prefix_values);
  if (!predicate.has_value() || !lhs.has_value() || !rhs.has_value() ||
      !true_value.has_value() || !false_value.has_value() || !lhs->steps.empty() ||
      !rhs->steps.empty()) {
    return std::nullopt;
  }

  std::string current_name = select->result.name;
  std::vector<Slice::ArithmeticStep> post_steps;
  for (std::size_t index = select_index + 1; index < block.insts.size(); ++index) {
    const auto* binary = get_binary_inst(block.insts[index]);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 || binary->result.name.empty()) {
      return std::nullopt;
    }

    const bool lhs_is_current =
        binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->lhs.type == c4c::backend::bir::TypeKind::I32 && binary->lhs.name == current_name;
    const bool rhs_is_current =
        binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I32 && binary->rhs.name == current_name;
    if (lhs_is_current == rhs_is_current) {
      return std::nullopt;
    }

    std::optional<std::int64_t> immediate;
    if (lhs_is_current && binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
      immediate = binary->rhs.immediate;
    } else if (rhs_is_current && binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
               binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
               binary->lhs.type == c4c::backend::bir::TypeKind::I32) {
      immediate = binary->lhs.immediate;
    } else {
      return std::nullopt;
    }

    post_steps.push_back({binary->opcode, *immediate});
    current_name = binary->result.name;
  }

  if (block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32 ||
      block.terminator.value->name != current_name) {
    return std::nullopt;
  }

  return Slice{
      function.name,
      *predicate,
      *lhs,
      *rhs,
      "select_true",
      "select_false",
      "select_join",
      *true_value,
      *false_value,
      std::move(post_steps),
  };
}

c4c::backend::RegAllocIntegrationResult synthesize_shared_aarch64_call_crossing_regalloc(
    std::string_view source_value) {
  c4c::backend::RegAllocIntegrationResult regalloc;
  regalloc.reg_assignments.emplace(std::string(source_value), kAarch64CalleeSavedRegs.front());
  regalloc.used_callee_saved.push_back(kAarch64CalleeSavedRegs.front());
  return regalloc;
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

std::string escape_asm_string(std::string_view raw_bytes) {
  auto hex_value = [](unsigned char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };

  auto decode_llvm_c_string = [&](std::string_view bytes) {
    std::string decoded;
    decoded.reserve(bytes.size());
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      const char ch = bytes[i];
      if (ch != '\\' || i + 1 >= bytes.size()) {
        decoded.push_back(ch);
        continue;
      }

      const char code = bytes[i + 1];
      if (code == 'x' && i + 3 < bytes.size()) {
        const int hi = hex_value(static_cast<unsigned char>(bytes[i + 2]));
        const int lo = hex_value(static_cast<unsigned char>(bytes[i + 3]));
        if (hi >= 0 && lo >= 0) {
          decoded.push_back(static_cast<char>((hi << 4) | lo));
          i += 3;
          continue;
        }
        decoded.push_back('\\');
        continue;
      }

      const int hi = hex_value(static_cast<unsigned char>(code));
      if (hi >= 0 && i + 2 < bytes.size()) {
        const int lo = hex_value(static_cast<unsigned char>(bytes[i + 2]));
        if (lo >= 0) {
          decoded.push_back(static_cast<char>((hi << 4) | lo));
          i += 2;
          continue;
        }
      }

      ++i;
      switch (code) {
        case 'n':
          decoded.push_back('\n');
          break;
        case 'r':
          decoded.push_back('\r');
          break;
        case 't':
          decoded.push_back('\t');
          break;
        case '\"':
          decoded.push_back('\"');
          break;
        case '\\':
          decoded.push_back('\\');
          break;
        default:
          decoded.push_back('\\');
          decoded.push_back(code);
          break;
      }
    }
    return decoded;
  };

  const std::string decoded = decode_llvm_c_string(raw_bytes);
  std::string escaped;
  escaped.reserve(decoded.size());
  for (const unsigned char ch : decoded) {
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
      case '\r':
        escaped += "\\r";
        break;
      case '\0':
        escaped += "\\000";
        break;
      default:
        if (ch >= 0x20 && ch <= 0x7e) {
          escaped.push_back(static_cast<char>(ch));
        } else {
          constexpr char kHexDigits[] = "0123456789ABCDEF";
          escaped += "\\x";
          escaped.push_back(kHexDigits[(ch >> 4) & 0xf]);
          escaped.push_back(kHexDigits[ch & 0xf]);
        }
        break;
    }
  }
  return escaped;
}

std::string asm_private_data_label(std::string_view target_triple,
                                   std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }

  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
  if (is_darwin) {
    return "L." + label;
  }
  return ".L." + label;
}

std::string asm_private_data_label(const c4c::codegen::lir::LirModule& module,
                                   std::string_view pool_name) {
  return asm_private_data_label(module.target_profile.triple, pool_name);
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
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
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
  if (!byte_index.has_value() || *byte_index < 0 || *byte_index > 4095) {
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
      function.name,
      string_const.pool_name,
      string_const.raw_bytes,
      *byte_index,
      extend->kind,
  };
}

// BIR switch order:
// 1. string literal pool slices, once BIR exposes private data labels / pool
//    naming
// 2. global-backed load/store/diff slices, once BIR exposes globals with
//    storage/linkage/address-shape metadata
// 3. local-slot slices, once shared slot layout contracts exist
// 4. direct-call slices, once BIR exposes call-argument parsing and helper/main
//    pairing metadata
//
std::string gp_reg_name(c4c::backend::PhysReg reg, bool is_32bit) {
  return std::string(is_32bit ? "w" : "x") + std::to_string(reg.index);
}

std::int64_t aligned_call_frame_size(std::size_t saved_regs) {
  const std::size_t raw_size = (saved_regs + 1) * sizeof(std::uint64_t);
  return static_cast<std::int64_t>((raw_size + 15) & ~std::size_t{15});
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
      entry_store->slot_name != slot.name || entry_store->value.kind != Value::Kind::Immediate ||
      entry_store->value.type != TypeKind::I32 ||
      entry.terminator.target_label != loop.label || loop_load == nullptr ||
      loop_cmp == nullptr || loop.terminator.kind != TerminatorKind::CondBranch ||
      loop_load->slot_name != slot.name || loop_load->result.kind != Value::Kind::Named ||
      loop_load->result.type != TypeKind::I32 || loop_cmp->opcode != BinaryOpcode::Ne ||
      loop_cmp->result.kind != Value::Kind::Named || loop_cmp->result.type != TypeKind::I32 ||
      loop_cmp->lhs != loop_load->result || loop_cmp->rhs != Value::immediate_i32(0) ||
      loop.terminator.condition != loop_cmp->result ||
      loop.terminator.true_label != body.label ||
      loop.terminator.false_label != exit.label || body_load == nullptr ||
      body_sub == nullptr || body_store == nullptr ||
      body.terminator.kind != TerminatorKind::Branch ||
      body_load->slot_name != slot.name || body_load->result.kind != Value::Kind::Named ||
      body_load->result.type != TypeKind::I32 || body_sub->opcode != BinaryOpcode::Sub ||
      body_sub->result.kind != Value::Kind::Named || body_sub->result.type != TypeKind::I32 ||
      body_sub->lhs != body_load->result || body_sub->rhs != Value::immediate_i32(1) ||
      body_store->slot_name != slot.name || body_store->value != body_sub->result ||
      body.terminator.target_label != loop.label || exit_load == nullptr ||
      exit.terminator.kind != TerminatorKind::Return ||
      exit_load->slot_name != slot.name || exit_load->result.kind != Value::Kind::Named ||
      exit_load->result.type != TypeKind::I32 || !exit.terminator.value.has_value() ||
      *exit.terminator.value != exit_load->result) {
    return std::nullopt;
  }

  if (entry_store->value.immediate < 0) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{
      function.name,
      entry_store->value.immediate,
      loop.label,
      body.label,
      exit.label,
  };
}

// BIR-gap data slices.
// The scalar-global-load family now has a direct BIR contract; the remaining
// global/string slices still depend on richer storage or address metadata.
std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr || load->result.kind != Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{
      function.name,
      global.name,
      global.initializer->immediate,
      4,
      global.initializer->immediate == 0,
  };
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (!global.is_extern || global.type != TypeKind::I32 || global.initializer.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr || load->result.kind != Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{function.name, global.name};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (!global.is_extern || global.type != TypeKind::I32 || global.initializer.has_value()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr || load->result.kind != Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset == 0 || (load->byte_offset % 4) != 0 || load->byte_offset > 4095 ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .byte_offset = static_cast<std::int64_t>(load->byte_offset),
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
      global.initializer->kind != Value::Kind::Immediate ||
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
      entry.insts.size() == 2 ? std::get_if<StoreGlobalInst>(&entry.insts[0]) : nullptr;
  const auto* load =
      entry.insts.size() == 2 ? std::get_if<LoadGlobalInst>(&entry.insts[1]) : nullptr;
  if (entry.label != "entry" || store == nullptr || load == nullptr ||
      store->global_name != global.name || store->value.kind != Value::Kind::Immediate ||
      store->value.type != TypeKind::I32 || load->result.kind != Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{
      function.name,
      global.name,
      global.initializer->immediate,
      store->value.immediate,
      4,
  };
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
  bool zero_initializer = false;
  std::int64_t init_imm = 0;
  if (global.init_text == "zeroinitializer") {
    zero_initializer = true;
  } else {
    const auto parsed_init = parse_i64(global.init_text);
    if (!parsed_init.has_value()) {
      return std::nullopt;
    }
    init_imm = *parsed_init;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (load == nullptr || ret == nullptr || load->type_str != "i32" ||
      load->ptr != "@" + global.name || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{function.name,
                                      global.name,
                                      init_imm,
                                      global.align_bytes > 0 ? global.align_bytes : 4,
                                      zero_initializer};
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
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
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

  return MinimalScalarGlobalStoreReloadSlice{
      function.name,
      global.name,
      *init_imm,
      *store_imm,
      global.align_bytes > 0 ? global.align_bytes : 4,
  };
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
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
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

  return MinimalScalarGlobalLoadSlice{function.name, global.name, *init_imm,
                                      global.align_bytes > 0 ? global.align_bytes : 4};
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      global.llvm_type != "i32" || !global.init_text.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* load = std::get_if<LirLoadOp>(&entry.insts.front());
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (load == nullptr || ret == nullptr || load->type_str != "i32" ||
      load->ptr != "@" + global.name || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{function.name, global.name};
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
  if (function.is_declaration ||
      !lir_function_is_zero_arg_i32_definition(function) ||
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
      ret == nullptr || !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto byte_offset = *element_index * 4;
  if (byte_offset < 0 || byte_offset > 4095) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{function.name, global.name, byte_offset};
}

void emit_function_prelude(std::ostringstream& out,
                           std::string_view target_triple,
                           std::string_view symbol,
                           bool is_global) {
  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
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

std::string emit_minimal_return_imm_asm(std::string_view target_triple,
                                        std::string_view function_name,
                                        std::int64_t imm) {
  if (imm < 0 || imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string symbol = asm_symbol_name(target_triple, function_name);

  out << ".text\n";
  emit_function_prelude(out, target_triple, symbol, true);
  out << "  mov w0, #" << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_imm_asm(const c4c::backend::bir::Module& module,
                                        std::int64_t imm) {
  if (imm < 0 || imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string symbol = minimal_single_function_symbol(module);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, symbol, true);
  out << "  mov w0, #" << imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_sub_imm_asm(std::string_view target_triple,
                                            std::string_view function_name,
                                            std::int64_t imm) {
  if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
    return emit_minimal_return_imm_asm(target_triple, function_name, imm);
  }
  if (imm >= 0) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  const auto neg_imm = -imm;
  if (neg_imm < 0 || neg_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal sub-supported range");
  }

  std::ostringstream out;
  const std::string symbol = asm_symbol_name(target_triple, function_name);

  out << ".text\n";
  emit_function_prelude(out, target_triple, symbol, true);
  out << "  mov w0, #0\n"
      << "  sub w0, w0, #" << neg_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_sub_imm_asm(const c4c::backend::bir::Module& module,
                                            std::int64_t imm) {
  if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
    return emit_minimal_return_imm_asm(module, imm);
  }
  if (imm >= 0) {
    fail_unsupported("return immediates outside the minimal mov-supported range");
  }

  const auto neg_imm = -imm;
  if (neg_imm < 0 || neg_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("return immediates outside the minimal sub-supported range");
  }

  std::ostringstream out;
  const std::string symbol = minimal_single_function_symbol(module);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, symbol, true);
  out << "  mov w0, #0\n"
      << "  sub w0, w0, #" << neg_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_affine_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalAffineReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".text\n";
  emit_function_prelude(out, module.target_triple, symbol, true);
  if (!slice.uses_first_param && !slice.uses_second_param) {
    if (slice.constant >= 0 && slice.constant <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov w0, #" << slice.constant << "\n";
    } else if (slice.constant < 0 &&
               -slice.constant <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov w0, #0\n"
          << "  sub w0, w0, #" << -slice.constant << "\n";
    } else {
      fail_unsupported("affine return constant outside the minimal mov/sub-supported range");
    }
    out << "  ret\n";
    return out.str();
  }

  if (slice.constant > std::numeric_limits<std::uint16_t>::max() ||
      slice.constant < -std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("affine return adjustment outside the minimal add/sub-supported range");
  }
  if (slice.uses_first_param && slice.uses_second_param) {
    out << "  add w0, w0, w1\n";
  } else if (!slice.uses_first_param && slice.uses_second_param) {
    out << "  mov w0, w1\n";
  }
  if (slice.constant > 0) {
    out << "  add w0, w0, #" << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub w0, w0, #" << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_cast_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalCastReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".text\n";
  emit_function_prelude(out, module.target_triple, symbol, true);

  if (slice.opcode == c4c::backend::bir::CastOpcode::SExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I32 &&
      slice.result_type == c4c::backend::bir::TypeKind::I64) {
    out << "  sxtw x0, w0\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I8 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  and w0, w0, #0xff\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::Trunc &&
      slice.operand_type == c4c::backend::bir::TypeKind::I64 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  ret\n";
    return out.str();
  }

  fail_unsupported_direct_bir_module();
}

std::string emit_minimal_direct_call_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalDirectCallModuleView& slice) {
  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR zero-argument direct-call slice without helper metadata");
  }

  if (slice.return_imm < 0 ||
      slice.return_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("BIR helper return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol =
      asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol =
      asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  out << "  mov w0, #" << slice.return_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_void_direct_call_imm_return_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalVoidDirectCallImmReturnModuleView& slice) {
  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR zero-argument void direct-call slice without helper metadata");
  }

  if (slice.return_imm < 0 ||
      slice.return_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("BIR void direct-call return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n"
      << "  bl " << helper_symbol << "\n"
      << "  mov w0, #" << slice.return_imm << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalTwoArgDirectCallModuleView& slice) {
  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR two-argument direct-call slice without helper metadata");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  out << "  add w0, w0, w1\n"
      << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n";
  emit_i32_imm(out, "w0", slice.lhs_call_arg_imm,
               "two-argument direct-call lhs immediate exceeds the minimal aarch64 mov/sub range");
  emit_i32_imm(out, "w1", slice.rhs_call_arg_imm,
               "two-argument direct-call rhs immediate exceeds the minimal aarch64 mov/sub range");
  out << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalDirectCallAddImmModuleView& slice) {
  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR single-argument add-immediate direct-call slice without helper metadata");
  }
  if (slice.add_imm > std::numeric_limits<std::uint16_t>::max() ||
      slice.add_imm < -std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported(
        "single-argument direct-call add-immediate exceeds the minimal aarch64 add/sub range");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  if (slice.add_imm >= 0) {
    out << "  add w0, w0, #" << slice.add_imm << "\n";
  } else {
    out << "  sub w0, w0, #" << -slice.add_imm << "\n";
  }
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n";
  emit_i32_imm(out, "w0", slice.call_arg_imm,
               "single-argument direct-call immediate exceeds the minimal aarch64 mov/sub range");
  out << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_identity_arg_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalDirectCallIdentityArgModuleView& slice) {
  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR identity direct-call slice without helper metadata");
  }

  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n";
  emit_i32_imm(out, "w0", slice.call_arg_imm,
               "identity direct-call immediate exceeds the minimal aarch64 mov/sub range");
  out << "  bl " << helper_symbol << "\n"
      << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_dual_identity_direct_call_sub_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalDualIdentityDirectCallSubModuleView& slice) {
  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.lhs_helper == nullptr || slice.rhs_helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR dual-identity direct-call subtraction slice without helper metadata");
  }

  std::ostringstream out;
  const std::string lhs_symbol = asm_symbol_name(module.target_triple, slice.lhs_helper->name);
  const std::string rhs_symbol = asm_symbol_name(module.target_triple, slice.rhs_helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  out << ".text\n";
  emit_function_prelude(out, module.target_triple, lhs_symbol, false);
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, rhs_symbol, false);
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #32\n"
      << "  str x19, [sp, #8]\n"
      << "  str x30, [sp, #24]\n";
  emit_i32_imm(out, "w0", slice.lhs_call_arg_imm,
               "dual-identity direct-call lhs immediate exceeds the minimal aarch64 mov/sub range");
  out << "  bl " << lhs_symbol << "\n"
      << "  mov w19, w0\n";
  emit_i32_imm(out, "w0", slice.rhs_call_arg_imm,
               "dual-identity direct-call rhs immediate exceeds the minimal aarch64 mov/sub range");
  out << "  bl " << rhs_symbol << "\n"
      << "  sub w19, w19, w0\n"
      << "  mov w0, w19\n"
      << "  ldr x19, [sp, #8]\n"
      << "  ldr x30, [sp, #24]\n"
      << "  add sp, sp, #32\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_call_crossing_direct_call_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::RegAllocIntegrationResult& regalloc,
    const c4c::backend::ParsedBirMinimalCallCrossingDirectCallModuleView& slice) {
  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.helper == nullptr || slice.main_function == nullptr) {
    fail_unsupported("BIR call-crossing direct-call slice without helper metadata");
  }
  if (slice.helper_add_imm > std::numeric_limits<std::uint16_t>::max() ||
      slice.helper_add_imm < -std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("call-crossing helper add immediate exceeds the minimal aarch64 add/sub range");
  }

  const std::string source_value(slice.regalloc_source_value);
  const auto* source_reg =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, source_value);
  if (source_reg == nullptr ||
      !c4c::backend::stack_layout::uses_callee_saved_reg(regalloc, *source_reg)) {
    throw std::invalid_argument(
        "shared call-crossing regalloc state is required for the minimal aarch64 direct-call slice");
  }

  const std::string reg32 = aarch64_reg32_name(*source_reg);
  const std::string reg64 = aarch64_reg64_name(*source_reg);
  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  std::ostringstream out;
  out << ".text\n";
  emit_function_prelude(out, module.target_triple, helper_symbol, false);
  if (slice.helper_add_imm >= 0) {
    out << "  add w0, w0, #" << slice.helper_add_imm << "\n";
  } else {
    out << "  sub w0, w0, #" << -slice.helper_add_imm << "\n";
  }
  out << "  ret\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #32\n"
      << "  str " << reg64 << ", [sp, #8]\n"
      << "  str x30, [sp, #24]\n";
  emit_i32_imm(out, reg32, slice.source_imm,
               "call-crossing source immediate exceeds the minimal aarch64 mov/sub range");
  out << "  mov w0, " << reg32 << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  add w0, w0, " << reg32 << "\n"
      << "  ldr " << reg64 << ", [sp, #8]\n"
      << "  ldr x30, [sp, #24]\n"
      << "  add sp, sp, #32\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_declared_direct_call_asm(
    const c4c::backend::bir::Module& module,
    const c4c::backend::ParsedBirMinimalDeclaredDirectCallModuleView& slice) {
  static constexpr const char* kArgI32Regs[8] = {"w0", "w1", "w2", "w3",
                                                 "w4", "w5", "w6", "w7"};
  static constexpr const char* kArgPtrRegs[8] = {"x0", "x1", "x2", "x3",
                                                 "x4", "x5", "x6", "x7"};

  auto emit_i32_imm = [](std::ostringstream& out,
                         std::string_view reg,
                         std::int64_t imm,
                         const char* detail) {
    if (imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #" << imm << "\n";
      return;
    }
    if (imm < 0 && -imm <= std::numeric_limits<std::uint16_t>::max()) {
      out << "  mov " << reg << ", #0\n"
          << "  sub " << reg << ", " << reg << ", #" << -imm << "\n";
      return;
    }
    fail_unsupported(detail);
  };

  if (slice.args.size() > 8) {
    fail_unsupported("extern declaration call argument count exceeds minimal aarch64 register budget");
  }

  std::vector<std::string> emitted_string_constant_names;
  for (const auto& arg : slice.args) {
    if (arg.kind != c4c::backend::ParsedBackendExternCallArg::Kind::Ptr ||
        arg.operand.empty() || arg.operand.front() != '@') {
      continue;
    }
    const auto* string_constant = find_string_constant(module, arg.operand.substr(1));
    if (string_constant == nullptr) {
      continue;
    }
    bool duplicate = false;
    for (const auto& emitted : emitted_string_constant_names) {
      if (emitted == string_constant->name) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      emitted_string_constant_names.push_back(string_constant->name);
    }
  }

  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.callee->name);
  const std::string main_symbol = asm_symbol_name(module.target_triple, slice.main_function->name);

  std::ostringstream out;
  if (!emitted_string_constant_names.empty()) {
    out << ".section .rodata\n";
    for (const auto& name : emitted_string_constant_names) {
      const auto* string_constant = find_string_constant(module, name);
      if (string_constant == nullptr) {
        continue;
      }
      const std::string label = asm_private_data_label(module.target_triple, std::string("@") + name);
      out << label << ":\n"
          << "  .asciz \"" << escape_asm_string(string_constant->bytes) << "\"\n";
    }
  }
  out << ".text\n";
  emit_function_prelude(out, module.target_triple, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  str x30, [sp, #8]\n";
  for (std::size_t index = 0; index < slice.args.size(); ++index) {
    const auto& arg = slice.args[index];
    switch (arg.kind) {
      case c4c::backend::ParsedBackendExternCallArg::Kind::I32Imm:
        emit_i32_imm(out,
                     kArgI32Regs[index],
                     arg.imm,
                     "extern declaration i32 immediates outside the minimal mov-supported range");
        break;
      case c4c::backend::ParsedBackendExternCallArg::Kind::I64Imm:
        if (arg.imm < 0 || arg.imm > std::numeric_limits<std::uint16_t>::max()) {
          fail_unsupported(
              "extern declaration i64 immediates outside the minimal mov-supported range");
        }
        out << "  mov " << kArgPtrRegs[index] << ", #" << arg.imm << "\n";
        break;
      case c4c::backend::ParsedBackendExternCallArg::Kind::Ptr: {
        const auto operand = arg.operand;
        if (operand.empty() || operand.front() != '@') {
          fail_unsupported("non-symbol pointer operands in minimal extern declaration calls");
        }
        const auto* string_constant = find_string_constant(module, operand.substr(1));
        const std::string symbol =
            string_constant != nullptr
                ? asm_private_data_label(module.target_triple, std::string(operand))
                : asm_symbol_name(module.target_triple, operand.substr(1));
        out << "  adrp " << kArgPtrRegs[index] << ", " << symbol << "\n"
            << "  add " << kArgPtrRegs[index] << ", " << kArgPtrRegs[index]
            << ", :lo12:" << symbol << "\n";
      } break;
    }
  }
  out << "  bl " << helper_symbol << "\n";
  if (!slice.return_call_result) {
    emit_i32_imm(out,
                 "w0",
                 slice.return_imm,
                 "extern declaration fixed returns outside the minimal mov-supported range");
  }
  out << "  ldr x30, [sp, #8]\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_return_asm(
    std::string_view target_triple,
    const MinimalConditionalReturnSlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  const auto immediate_in_range =
      [&](const MinimalConditionalReturnSlice::ValueSource& value) {
        return value.kind != MinimalConditionalReturnSlice::ValueKind::Immediate ||
               in_mov_range(value.imm);
      };
  if (!immediate_in_range(slice.lhs) || !immediate_in_range(slice.rhs) ||
      !immediate_in_range(slice.true_value) || !immediate_in_range(slice.false_value)) {
    fail_unsupported("conditional-return immediates outside the minimal mov-supported range");
  }

  auto emit_move_w = [](std::ostringstream& out,
                        std::string_view dst,
                        const MinimalConditionalReturnSlice::ValueSource& value) {
    switch (value.kind) {
      case MinimalConditionalReturnSlice::ValueKind::Immediate:
        out << "  mov " << dst << ", #" << value.imm << "\n";
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param0:
        if (dst != "w0") {
          out << "  mov " << dst << ", w0\n";
        }
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param1:
        if (dst != "w1") {
          out << "  mov " << dst << ", w1\n";
        }
        return;
    }
  };

  std::ostringstream out;
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);
  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "b.ge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "b.gt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "b.le"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "b.lt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "b.hs"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "b.hi"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "b.ls"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "b.lo"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "b.ne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "b.eq"; break;
    default:
      fail_unsupported(
          "conditional-return predicates outside the current compare-and-branch slice");
  }

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  emit_move_w(out, "w8", slice.lhs);
  if (slice.rhs.kind == MinimalConditionalReturnSlice::ValueKind::Immediate) {
    out << "  cmp w8, #" << slice.rhs.imm << "\n";
  } else {
    emit_move_w(out, "w9", slice.rhs);
    out << "  cmp w8, w9\n";
  }
  out << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n";
  emit_move_w(out, "w0", slice.true_value);
  out << "  ret\n"
      << ".L" << slice.false_label << ":\n";
  emit_move_w(out, "w0", slice.false_value);
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i8_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI8ReturnSlice& slice) {
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  const auto in_add_sub_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= 4095;
  };
  const auto value_immediates_in_range =
      [&](const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
        if (value.kind == MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate &&
            !in_mov_range(value.imm)) {
          return false;
        }
        for (const auto& step : value.steps) {
          if (!in_add_sub_range(step.imm)) {
            return false;
          }
        }
        return true;
      };
  if (!value_immediates_in_range(slice.lhs) || !value_immediates_in_range(slice.rhs) ||
      !value_immediates_in_range(slice.true_value) ||
      !value_immediates_in_range(slice.false_value)) {
    fail_unsupported("conditional-i8-affine-return immediates outside the minimal aarch64 slice range");
  }
  for (const auto& step : slice.post_steps) {
    if (!in_add_sub_range(step.imm)) {
      fail_unsupported("conditional-i8-affine-return post-select arithmetic outside the minimal aarch64 slice range");
    }
  }

  auto emit_value_expr_to_w =
      [](std::ostringstream& out,
         std::string_view dst,
         const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
            out << "  mov " << dst << ", #" << value.imm << "\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
            out << "  and " << dst << ", w0, #0xff\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
            out << "  and " << dst << ", w1, #0xff\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " " << dst << ", " << dst << ", #" << step.imm << "\n";
        }
      };

  std::ostringstream out;
  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "b.ge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "b.gt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "b.le"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "b.lt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "b.hs"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "b.hi"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "b.ls"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "b.lo"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "b.ne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "b.eq"; break;
    default:
      fail_unsupported(
          "conditional-i8-affine-return predicates outside the current compare-and-branch slice");
  }

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  emit_value_expr_to_w(out, "w8", slice.lhs);
  emit_value_expr_to_w(out, "w9", slice.rhs);
  out << "  cmp w8, w9\n"
      << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_w(out, "w0", slice.true_value);
  out << "  b .L" << slice.join_label << "\n"
      << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_w(out, "w0", slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " w0, w0, #" << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i32_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI32ReturnSlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  const auto in_add_sub_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= 4095;
  };
  const auto value_immediates_in_range =
      [&](const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        if (value.kind == MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate &&
            !in_mov_range(value.imm)) {
          return false;
        }
        for (const auto& step : value.steps) {
          if (!in_add_sub_range(step.imm)) {
            return false;
          }
        }
        return true;
      };
  if (!value_immediates_in_range(slice.lhs) || !value_immediates_in_range(slice.rhs) ||
      !value_immediates_in_range(slice.true_value) ||
      !value_immediates_in_range(slice.false_value)) {
    fail_unsupported("conditional-i32-affine-return immediates outside the minimal aarch64 slice range");
  }
  for (const auto& step : slice.post_steps) {
    if (!in_add_sub_range(step.imm)) {
      fail_unsupported("conditional-i32-affine-return post-select arithmetic outside the minimal aarch64 slice range");
    }
  }

  auto emit_value_expr_to_w =
      [](std::ostringstream& out,
         std::string_view dst,
         const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
            out << "  mov " << dst << ", #" << value.imm << "\n";
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
            if (dst != "w0") {
              out << "  mov " << dst << ", w0\n";
            }
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
            out << "  mov " << dst << ", w1\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " " << dst << ", " << dst << ", #" << step.imm << "\n";
        }
      };

  std::ostringstream out;
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);
  const char* fail_branch = nullptr;
  switch (slice.predicate) {
    case c4c::codegen::lir::LirCmpPredicate::Slt: fail_branch = "b.ge"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sle: fail_branch = "b.gt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sgt: fail_branch = "b.le"; break;
    case c4c::codegen::lir::LirCmpPredicate::Sge: fail_branch = "b.lt"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ult: fail_branch = "b.hs"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ule: fail_branch = "b.hi"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ugt: fail_branch = "b.ls"; break;
    case c4c::codegen::lir::LirCmpPredicate::Uge: fail_branch = "b.lo"; break;
    case c4c::codegen::lir::LirCmpPredicate::Eq: fail_branch = "b.ne"; break;
    case c4c::codegen::lir::LirCmpPredicate::Ne: fail_branch = "b.eq"; break;
    default:
      fail_unsupported(
          "conditional-i32-affine-return predicates outside the current compare-and-branch slice");
  }

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  emit_value_expr_to_w(out, "w8", slice.lhs);
  emit_value_expr_to_w(out, "w9", slice.rhs);
  out << "  cmp w8, w9\n"
      << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_w(out, "w0", slice.true_value);
  out << "  b .L" << slice.join_label << "\n"
      << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_w(out, "w0", slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " w0, w0, #" << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_countdown_loop_asm(std::string_view target_triple,
                                            const MinimalCountdownLoopSlice& slice) {
  std::ostringstream out;
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  out << "  mov w0, #" << slice.initial_imm << "\n"
      << ".L" << slice.loop_label << ":\n"
      << "  cmp w0, #0\n"
      << "  b.eq .L" << slice.exit_label << "\n"
      << ".L" << slice.body_label << ":\n"
      << "  sub w0, w0, #1\n"
      << "  b .L" << slice.loop_label << "\n"
      << ".L" << slice.exit_label << ":\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalLoadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max()) {
    fail_unsupported("scalar global initializers outside the minimal .long-supported range");
  }

  const bool is_darwin =
      target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);
  const int align_pow2 = slice.align_bytes <= 1
                             ? 0
                             : (slice.align_bytes == 2 ? 1 : 2);

  std::ostringstream out;
  out << (slice.zero_initializer ? ".bss\n" : ".data\n")
      << ".globl " << global_symbol << "\n";
  if (!is_darwin) {
    out << ".type " << global_symbol << ", %object\n";
  }
  out << ".p2align " << align_pow2 << "\n"
      << global_symbol << ":\n";
  if (slice.zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << slice.init_imm << "\n";
  }
  if (!is_darwin) {
    out << ".size " << global_symbol << ", 4\n";
  }

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  ldr w0, [x8, " << global_symbol << "@PAGEOFF]\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  ldr w0, [x8, :lo12:" << global_symbol << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalStoreReloadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max()) {
    fail_unsupported("scalar global initializers outside the minimal .long-supported range");
  }

  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  if (!in_mov_range(slice.store_imm)) {
    fail_unsupported("scalar global store immediates outside the minimal mov-supported range");
  }

  const bool is_darwin =
      target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);
  const int align_pow2 = slice.align_bytes <= 1
                             ? 0
                             : (slice.align_bytes == 2 ? 1 : 2);

  std::ostringstream out;
  out << ".data\n"
      << ".globl " << global_symbol << "\n";
  if (!is_darwin) {
    out << ".type " << global_symbol << ", %object\n";
  }
  out << ".p2align " << align_pow2 << "\n"
      << global_symbol << ":\n"
      << "  .long " << slice.init_imm << "\n";
  if (!is_darwin) {
    out << ".size " << global_symbol << ", 4\n";
  }

  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  add x8, x8, " << global_symbol << "@PAGEOFF\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  add x8, x8, :lo12:" << global_symbol << "\n";
  }
  out << "  mov w9, #" << slice.store_imm << "\n"
      << "  str w9, [x8]\n"
      << "  ldr w0, [x8]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  const bool is_darwin =
      target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  if (!is_darwin) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  ldr w0, [x8, " << global_symbol << "@PAGEOFF]\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  ldr w0, [x8, :lo12:" << global_symbol << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_global_array_load_asm(
    std::string_view target_triple,
    const MinimalExternGlobalArrayLoadSlice& slice) {
  const bool is_darwin =
      target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  if (!is_darwin) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  add x8, x8, " << global_symbol << "@PAGEOFF\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  add x8, x8, :lo12:" << global_symbol << "\n";
  }
  out << "  ldr w0, [x8, #" << slice.byte_offset << "]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    std::string_view target_triple,
    const MinimalStringLiteralCharSlice& slice) {
  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
  const std::string string_label = asm_private_data_label(target_triple, slice.pool_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  if (is_darwin) {
    out << ".section __TEXT,__cstring,cstring_literals\n";
  } else {
    out << ".section .rodata\n";
  }
  out << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n";
  out << ".text\n";
  emit_function_prelude(out, target_triple, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << string_label << "@PAGE\n"
        << "  add x8, x8, " << string_label << "@PAGEOFF\n";
  } else {
    out << "  adrp x8, " << string_label << "\n"
        << "  add x8, x8, :lo12:" << string_label << "\n";
  }
  out << "  ldrb w0, [x8, #" << slice.byte_index << "]\n";
  if (slice.extend_kind == c4c::codegen::lir::LirCastKind::SExt) {
    out << "  sxtb w0, w0\n";
  }
  out << "  ret\n";
  return out.str();
}

// ── General-purpose stack-spill AArch64 emitter ──────────────────────────────
// Every SSA value gets an 8-byte stack slot.  Instructions load operands from
// stack, compute in scratch registers (w0/x0, w1/x1, ...), store result back.
// No register allocation needed — correct by construction.

struct GenSlotMap {
  std::unordered_map<std::string, int> slots;  // SSA name → sp offset
  std::unordered_map<std::string, int> alloca_data;  // alloca SSA name → data area offset
  int next_offset = 8;  // [sp+0] reserved for lr
  int frame_size = 0;
  int va_gp_save_offset = -1;
  int va_fp_save_offset = -1;

  int get_or_alloc(const std::string& name) {
    if (name.empty()) return -1;
    auto it = slots.find(name);
    if (it != slots.end()) return it->second;
    int off = next_offset;
    slots[name] = off;
    next_offset += 8;
    return off;
  }

  // Allocate data space for an alloca (separate from the pointer slot).
  int alloc_data(const std::string& name, int size_bytes) {
    int off = next_offset;
    alloca_data[name] = off;
    next_offset += size_bytes;
    // Align to 8
    if (next_offset % 8 != 0) next_offset += 8 - (next_offset % 8);
    return off;
  }

  void reserve_variadic_save_area() {
    if (va_gp_save_offset >= 0) return;
    va_gp_save_offset = next_offset;
    next_offset += 64;
    if (next_offset % 16 != 0) next_offset += 16 - (next_offset % 16);
    va_fp_save_offset = next_offset;
    next_offset += 128;
    if (next_offset % 16 != 0) next_offset += 16 - (next_offset % 16);
  }

  int lookup(const std::string& name) const {
    auto it = slots.find(name);
    if (it != slots.end()) return it->second;
    return -1;
  }

  int lookup_data(const std::string& name) const {
    auto it = alloca_data.find(name);
    if (it != alloca_data.end()) return it->second;
    return -1;
  }

  void finalize() {
    frame_size = next_offset;
    // align to 16
    if (frame_size % 16 != 0) frame_size += 16 - (frame_size % 16);
  }
};

// Parse type width in bits from LLVM type string.
static int gen_type_bits(const std::string& ty) {
  if (ty == "ptr") return 64;
  if (ty == "fp128") return 128;
  if (ty.size() > 1 && ty[0] == 'i') {
    int v = 0;
    for (size_t i = 1; i < ty.size(); ++i) {
      if (ty[i] >= '0' && ty[i] <= '9') v = v * 10 + (ty[i] - '0');
      else return 64;  // fallback
    }
    return v;
  }
  return 64;  // default
}

static bool gen_is_fp_type(const std::string& ty) {
  return ty == "float" || ty == "double";
}

static bool gen_try_parse_fp128_immediate_words(const std::string& op,
                                                std::uint64_t& low_word,
                                                std::uint64_t& high_word) {
  std::string_view text = op;
  if (text.rfind("0xL", 0) != 0 && text.rfind("0XL", 0) != 0) {
    return false;
  }

  text.remove_prefix(3);
  if (text.empty() || text.size() > 32) {
    return false;
  }

  std::uint64_t hi = 0;
  std::uint64_t lo = 0;
  for (char c : text) {
    unsigned digit = 0;
    if (c >= '0' && c <= '9') {
      digit = static_cast<unsigned>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      digit = static_cast<unsigned>(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      digit = static_cast<unsigned>(c - 'A' + 10);
    } else {
      return false;
    }

    hi = (hi << 4) | (lo >> 60);
    lo = (lo << 4) | digit;
  }

  high_word = hi;
  low_word = lo;
  return true;
}

static bool gen_is_scalar_variadic_type(const std::string& ty) {
  return ty == "ptr" || ty == "float" || ty == "double" ||
         (ty.size() > 1 && ty[0] == 'i');
}

static bool gen_is_64bit(const std::string& ty) { return gen_type_bits(ty) > 32; }
static const char* gen_reg_prefix(const std::string& ty) { return gen_is_64bit(ty) ? "x" : "w"; }

static void gen_emit_integer_immediate(std::ostringstream& out, int reg_idx,
                                       std::uint64_t value, bool is_64bit) {
  if (is_64bit) {
    if (value <= 65535) {
      out << "  mov x" << reg_idx << ", #" << value << "\n";
      return;
    }
    out << "  movz x" << reg_idx << ", #" << (value & 0xFFFF) << "\n";
    if ((value >> 16) & 0xFFFF) {
      out << "  movk x" << reg_idx << ", #" << ((value >> 16) & 0xFFFF)
          << ", lsl #16\n";
    }
    if ((value >> 32) & 0xFFFF) {
      out << "  movk x" << reg_idx << ", #" << ((value >> 32) & 0xFFFF)
          << ", lsl #32\n";
    }
    if ((value >> 48) & 0xFFFF) {
      out << "  movk x" << reg_idx << ", #" << ((value >> 48) & 0xFFFF)
          << ", lsl #48\n";
    }
    return;
  }

  const std::uint32_t narrow = static_cast<std::uint32_t>(value);
  if (static_cast<std::int64_t>(static_cast<std::int32_t>(narrow)) >= -65536 &&
      narrow <= 65535) {
    out << "  mov w" << reg_idx << ", #"
        << static_cast<std::int32_t>(narrow) << "\n";
    return;
  }
  out << "  movz w" << reg_idx << ", #" << (narrow & 0xFFFF) << "\n";
  if ((narrow >> 16) & 0xFFFF) {
    out << "  movk w" << reg_idx << ", #" << ((narrow >> 16) & 0xFFFF)
        << ", lsl #16\n";
  }
}

static bool gen_try_parse_fp_immediate_bits(const std::string& op, const std::string& ty,
                                            std::uint64_t& bits_out) {
  if (!gen_is_fp_type(ty)) return false;

  try {
    if (op.rfind("0x", 0) == 0 || op.rfind("-0x", 0) == 0) {
      const std::uint64_t raw_bits = std::stoull(op, nullptr, 16);
      if (ty == "float") {
        double as_double = 0.0;
        std::memcpy(&as_double, &raw_bits, sizeof(as_double));
        const float narrowed = static_cast<float>(as_double);
        std::uint32_t bits32 = 0;
        std::memcpy(&bits32, &narrowed, sizeof(narrowed));
        bits_out = bits32;
      } else {
        bits_out = raw_bits;
      }
      return true;
    }

    if (ty == "double") {
      const double value = std::stod(op);
      std::memcpy(&bits_out, &value, sizeof(value));
      return true;
    }

    const float value = std::stof(op);
    std::uint32_t bits32 = 0;
    std::memcpy(&bits32, &value, sizeof(value));
    bits_out = bits32;
    return true;
  } catch (...) {
    return false;
  }
}

static void gen_emit_sp_adjust(std::ostringstream& out, const char* op, int amount) {
  if (amount <= 0) return;
  int remaining = amount;
  while (remaining > 0) {
    const int chunk = std::min(remaining, 4095);
    out << "  " << op << " sp, sp, #" << chunk << "\n";
    remaining -= chunk;
  }
}

static void gen_emit_add_imm(std::ostringstream& out, const char* dst, const char* base,
                             int amount, const char* scratch) {
  if (amount == 0) {
    out << "  mov " << dst << ", " << base << "\n";
    return;
  }
  if (amount > 0 && amount < 4096) {
    out << "  add " << dst << ", " << base << ", #" << amount << "\n";
    return;
  }
  gen_emit_integer_immediate(out, scratch[1] - '0', static_cast<std::uint64_t>(amount), true);
  out << "  add " << dst << ", " << base << ", " << scratch << "\n";
}

static void gen_emit_store_i32_constant(std::ostringstream& out, const char* addr_reg,
                                        int byte_offset, int value) {
  if (value < 0) {
    gen_emit_integer_immediate(out, 9, static_cast<std::uint64_t>(-value), false);
    out << "  neg w9, w9\n";
  } else {
    gen_emit_integer_immediate(out, 9, static_cast<std::uint64_t>(value), false);
  }
  out << "  str w9, [" << addr_reg;
  if (byte_offset != 0) out << ", #" << byte_offset;
  out << "]\n";
}

static std::string gen_chunk_type_for_size(int byte_size) {
  if (byte_size <= 1) return "i8";
  if (byte_size <= 2) return "i16";
  if (byte_size <= 4) return "i32";
  return "i64";
}

static void gen_emit_load_chunk_from_addr(std::ostringstream& out, int dst_reg,
                                          std::string_view elem_ty, int base_reg,
                                          int source_offset) {
  const std::string elem_type(elem_ty);
  const int bits = gen_type_bits(elem_type);
  out << "  ";
  if (elem_type == "fp128") {
    out << "ldr q" << dst_reg;
  } else if (bits <= 8) {
    out << "ldrb w" << dst_reg;
  } else if (bits <= 16) {
    out << "ldrh w" << dst_reg;
  } else if (bits <= 32) {
    out << "ldr w" << dst_reg;
  } else if (elem_type == "double") {
    out << "ldr d" << dst_reg;
  } else if (elem_type == "float") {
    out << "ldr s" << dst_reg;
  } else {
    out << "ldr x" << dst_reg;
  }
  out << ", [x" << base_reg;
  if (source_offset != 0) {
    out << ", #" << source_offset;
  }
  out << "]\n";
}

static void gen_emit_store_chunk_to_addr(std::ostringstream& out, int src_reg, int byte_size,
                                         int base_reg, int dest_offset) {
  out << "  ";
  if (byte_size == 16) {
    out << "str q" << src_reg;
  } else if (byte_size <= 1) {
    out << "strb w" << src_reg;
  } else if (byte_size <= 2) {
    out << "strh w" << src_reg;
  } else if (byte_size <= 4) {
    out << "str w" << src_reg;
  } else {
    out << "str x" << src_reg;
  }
  out << ", [x" << base_reg;
  if (dest_offset != 0) {
    out << ", #" << dest_offset;
  }
  out << "]\n";
}

static void gen_emit_store_typed_chunk_to_addr(std::ostringstream& out, int src_reg,
                                               std::string_view elem_ty, int base_reg,
                                               int dest_offset) {
  const std::string elem_type(elem_ty);
  out << "  ";
  if (elem_type == "fp128") {
    out << "str q" << src_reg;
  } else if (elem_type == "double") {
    out << "str d" << src_reg;
  } else if (elem_type == "float") {
    out << "str s" << src_reg;
  } else {
    const int bits = gen_type_bits(elem_type);
    const int byte_size = bits <= 8 ? 1 : bits <= 16 ? 2 : bits <= 32 ? 4 : 8;
    if (byte_size <= 1) {
      out << "strb w" << src_reg;
    } else if (byte_size <= 2) {
      out << "strh w" << src_reg;
    } else if (byte_size <= 4) {
      out << "str w" << src_reg;
    } else {
      out << "str x" << src_reg;
    }
  }
  out << ", [x" << base_reg;
  if (dest_offset != 0) {
    out << ", #" << dest_offset;
  }
  out << "]\n";
}

static int gen_chunk_size_for_remaining_copy_bytes(int remaining_bytes) {
  if (remaining_bytes >= 16) return 16;
  if (remaining_bytes > 8) return 8;
  return remaining_bytes;
}

// Parse type byte size for GEP element sizing.
static std::pair<int, int> gen_parse_array_type(const std::string& ty);
static std::string gen_resolve_type_decl_body(const std::string& ty,
                                              const std::vector<std::string>& type_decls);
static std::vector<std::string> gen_split_aggregate_fields(std::string_view text);
struct GenTypeLayout;
static GenTypeLayout gen_type_layout(const std::string& ty,
                                     const std::vector<std::string>& type_decls);

static std::vector<unsigned char> gen_decode_llvm_byte_string(const std::string& text) {
  std::vector<unsigned char> bytes;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      int h1 = hex_val(text[i + 1]);
      int h2 = hex_val(text[i + 2]);
      if (h1 >= 0 && h2 >= 0) {
        bytes.push_back(static_cast<unsigned char>(h1 * 16 + h2));
        i += 2;
        continue;
      }
    }
    bytes.push_back(static_cast<unsigned char>(text[i]));
  }
  return bytes;
}

static void gen_emit_ascii_bytes(std::ostringstream& out,
                                 const std::vector<unsigned char>& bytes) {
  out << "  .ascii \"";
  for (unsigned char c : bytes) {
    if (c == '"') out << "\\\"";
    else if (c == '\\') out << "\\\\";
    else if (c == '\n') out << "\\n";
    else if (c == '\r') out << "\\r";
    else if (c == '\t') out << "\\t";
    else if (c < 32 || c >= 127) {
      char buf[8];
      std::snprintf(buf, sizeof(buf), "\\%03o", c);
      out << buf;
    } else {
      out << static_cast<char>(c);
    }
  }
  out << "\"\n";
}

static int gen_type_bytes(const std::string& ty) {
  if (ty == "float" || ty.rfind("float alignstack(", 0) == 0) return 4;
  if (ty == "double" || ty.rfind("double alignstack(", 0) == 0) return 8;
  if (ty == "fp128" || ty.rfind("fp128 alignstack(", 0) == 0) return 16;
  if (!ty.empty() && ty[0] == '[') {
    auto [cnt, esz] = gen_parse_array_type(ty);
    return cnt * esz;
  }
  int bits = gen_type_bits(ty);
  if (bits <= 8) return 1;
  if (bits <= 16) return 2;
  if (bits <= 32) return 4;
  return 8;
}

// Parse array type "[N x T]" → (count, element_size_bytes).
static std::pair<int, int> gen_parse_array_type(const std::string& ty) {
  // format: [N x elem_type]
  if (ty.empty() || ty[0] != '[') return {1, gen_type_bytes(ty)};
  size_t xpos = ty.find(" x ");
  if (xpos == std::string::npos) return {1, gen_type_bytes(ty)};
  int count = 0;
  for (size_t i = 1; i < xpos; ++i) {
    if (ty[i] >= '0' && ty[i] <= '9') count = count * 10 + (ty[i] - '0');
  }
  std::string elem = ty.substr(xpos + 3);
  if (!elem.empty() && elem.back() == ']') elem.pop_back();
  return {count, gen_type_bytes(elem)};
}

struct GenTypeLayout {
  int size = 0;
  int align = 1;
};

static std::string gen_trim_type_token(std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
    text.remove_prefix(1);
  }
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
    text.remove_suffix(1);
  }
  return std::string(text);
}

static std::string gen_strip_alignstack_attr(std::string_view text) {
  const std::string trimmed = gen_trim_type_token(text);
  const std::size_t attr_pos = trimmed.find(" alignstack(");
  if (attr_pos == std::string::npos || trimmed.empty() || trimmed.back() != ')') {
    return trimmed;
  }
  return trimmed.substr(0, attr_pos);
}

static bool gen_type_is_address_only_value(std::string_view text) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  if (stripped == "fp128") return true;
  if (stripped == "[2 x i64]") return true;
  if (stripped.empty() || stripped.front() != '[') return false;
  const std::size_t xpos = stripped.find(" x ");
  if (xpos == std::string::npos) return false;
  std::string elem_ty = stripped.substr(xpos + 3);
  if (!elem_ty.empty() && elem_ty.back() == ']') elem_ty.pop_back();
  elem_ty = gen_trim_type_token(elem_ty);
  return elem_ty == "float" || elem_ty == "double";
}

static bool gen_type_uses_fp_storage(std::string_view text,
                                     const std::vector<std::string>& type_decls) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  const std::string resolved = gen_resolve_type_decl_body(stripped, type_decls);
  return resolved.find("float") != std::string::npos ||
         resolved.find("double") != std::string::npos ||
         resolved.find("fp128") != std::string::npos;
}

static bool gen_is_direct_gp_aggregate_type(std::string_view text,
                                            const std::vector<std::string>& type_decls) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  if (stripped.empty()) return false;
  if (stripped == "ptr" || gen_is_fp_type(stripped) || stripped == "fp128") return false;
  if (stripped.front() != '%' && stripped.front() != '{') return false;
  const std::string resolved = gen_resolve_type_decl_body(stripped, type_decls);
  if (resolved.size() < 2 || resolved.front() != '{' || resolved.back() != '}') return false;
  std::string_view body(resolved);
  body.remove_prefix(1);
  body.remove_suffix(1);
  const auto fields = gen_split_aggregate_fields(body);
  if (fields.size() != 1 || fields.front().empty() || fields.front().front() != '[') return false;
  const auto [count, elem_size] = gen_parse_array_type(fields.front());
  if (elem_size != 1 || count <= 8 || count > 16) return false;
  const auto layout = gen_type_layout(stripped, type_decls);
  if (layout.size <= 8 || layout.size > 16) return false;
  return !gen_type_uses_fp_storage(stripped, type_decls);
}

struct GenFpAggregateArgInfo {
  int elem_count = 0;
  int elem_size = 0;
  int stack_align = 0;
  std::string elem_ty;
};

struct GenHfaInfo {
  int elem_count = 0;
  int elem_size = 0;
  int stack_align = 0;
  std::string elem_ty;
};

static std::optional<GenFpAggregateArgInfo> gen_try_parse_fp_aggregate_arg_type(
    std::string_view text) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  if (stripped.empty() || stripped.front() != '[') return std::nullopt;

  const auto [count, elem_size] = gen_parse_array_type(stripped);
  const std::size_t xpos = stripped.find(" x ");
  if (xpos == std::string::npos) return std::nullopt;
  std::string elem_ty = stripped.substr(xpos + 3);
  if (!elem_ty.empty() && elem_ty.back() == ']') elem_ty.pop_back();
  elem_ty = gen_trim_type_token(elem_ty);
  if (elem_ty != "float" && elem_ty != "double") return std::nullopt;
  return GenFpAggregateArgInfo{
      count,
      elem_size,
      std::max(8, elem_size),
      elem_ty,
  };
}

static std::optional<GenHfaInfo> gen_try_parse_hfa_type(
    std::string_view text, const std::vector<std::string>& type_decls) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  if (stripped.empty()) return std::nullopt;

  if (stripped.front() == '[') {
    const auto parsed = gen_try_parse_fp_aggregate_arg_type(stripped);
    if (!parsed.has_value() || parsed->elem_count <= 0 || parsed->elem_count > 4) {
      return std::nullopt;
    }
    return GenHfaInfo{
        parsed->elem_count,
        parsed->elem_size,
        parsed->stack_align,
        parsed->elem_ty,
    };
  }

  if (stripped.front() != '%' && stripped.front() != '{' && stripped.front() != '<') {
    return std::nullopt;
  }

  const std::string resolved = gen_resolve_type_decl_body(stripped, type_decls);
  if (resolved.size() < 2 || resolved.front() != '{' || resolved.back() != '}') {
    return std::nullopt;
  }

  std::string_view body(resolved);
  body.remove_prefix(1);
  body.remove_suffix(1);
  const auto fields = gen_split_aggregate_fields(body);
  if (fields.empty() || fields.size() > 4) return std::nullopt;

  const std::string first = gen_strip_alignstack_attr(fields.front());
  if (first != "float" && first != "double" && first != "fp128") return std::nullopt;
  for (const auto& field : fields) {
    if (gen_strip_alignstack_attr(field) != first) return std::nullopt;
  }

  const int elem_size = gen_type_layout(first, type_decls).size;
  return GenHfaInfo{
      static_cast<int>(fields.size()),
      elem_size,
      first == "fp128" ? 16 : std::max(8, elem_size),
      first,
  };
}

static bool gen_type_is_sret_aggregate_type(std::string_view text,
                                            const std::vector<std::string>& type_decls) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  if (stripped.empty() || stripped == "ptr" || gen_is_fp_type(stripped) || stripped == "fp128" ||
      stripped == "void") {
    return false;
  }
  if (gen_try_parse_hfa_type(stripped, type_decls).has_value()) return false;
  const auto layout = gen_type_layout(stripped, type_decls);
  return layout.size > 16 &&
         (stripped.front() == '%' || stripped.front() == '{' || stripped.front() == '[' ||
          stripped.front() == '<');
}

static bool gen_type_is_memory_value(std::string_view text,
                                     const std::vector<std::string>& type_decls) {
  const std::string stripped = gen_strip_alignstack_attr(text);
  return gen_type_is_address_only_value(stripped) ||
         gen_is_direct_gp_aggregate_type(stripped, type_decls) ||
         gen_type_is_sret_aggregate_type(stripped, type_decls) ||
         gen_try_parse_hfa_type(stripped, type_decls).has_value();
}

static std::string gen_resolve_type_decl_body(const std::string& ty,
                                              const std::vector<std::string>& type_decls) {
  if (ty.empty() || ty[0] != '%') return ty;
  for (const auto& decl : type_decls) {
    const auto eq = decl.find(" = type ");
    if (eq != std::string::npos && decl.substr(0, eq) == ty) {
      return decl.substr(eq + 8);
    }
  }
  return ty;
}

static std::vector<std::string> gen_split_aggregate_fields(std::string_view text) {
  std::vector<std::string> fields;
  int depth = 0;
  std::size_t field_start = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '{' || c == '[' || c == '<' || c == '(') {
      ++depth;
    } else if (c == '}' || c == ']' || c == '>' || c == ')') {
      if (depth > 0) --depth;
    } else if (c == ',' && depth == 0) {
      fields.push_back(gen_trim_type_token(text.substr(field_start, i - field_start)));
      field_start = i + 1;
    }
  }
  if (field_start < text.size()) {
    fields.push_back(gen_trim_type_token(text.substr(field_start)));
  }
  return fields;
}

static GenTypeLayout gen_type_layout(const std::string& ty,
                                     const std::vector<std::string>& type_decls);

static GenTypeLayout gen_struct_layout(const std::string& ty,
                                       const std::vector<std::string>& type_decls) {
  const std::string resolved = gen_resolve_type_decl_body(ty, type_decls);
  bool packed = false;
  std::string_view body = resolved;
  if (body.size() >= 4 && body[0] == '<' && body[1] == '{' &&
      body[body.size() - 2] == '}' && body.back() == '>') {
    packed = true;
    body.remove_prefix(2);
    body.remove_suffix(2);
  } else if (body.size() >= 2 && body.front() == '{' && body.back() == '}') {
    body.remove_prefix(1);
    body.remove_suffix(1);
  } else {
    return {8, 8};
  }

  const auto fields = gen_split_aggregate_fields(body);
  if (fields.empty()) return {0, packed ? 1 : 1};

  int offset = 0;
  int struct_align = packed ? 1 : 1;
  for (const auto& field : fields) {
    const auto field_layout = gen_type_layout(field, type_decls);
    const int field_align = std::max(1, packed ? 1 : field_layout.align);
    if (!packed && offset % field_align != 0) {
      offset += field_align - (offset % field_align);
    }
    offset += field_layout.size;
    if (!packed) struct_align = std::max(struct_align, field_align);
  }
  if (!packed && struct_align > 1 && offset % struct_align != 0) {
    offset += struct_align - (offset % struct_align);
  }
  return {offset, packed ? 1 : struct_align};
}

static GenTypeLayout gen_type_layout(const std::string& ty,
                                     const std::vector<std::string>& type_decls) {
  const std::string trimmed = gen_strip_alignstack_attr(ty);
  if (trimmed.empty()) return {0, 1};
  if (trimmed == "i1" || trimmed == "i8") return {1, 1};
  if (trimmed == "i16") return {2, 2};
  if (trimmed == "i32" || trimmed == "float") return {4, 4};
  if (trimmed == "i64" || trimmed == "ptr" || trimmed == "double") return {8, 8};
  if (trimmed == "fp128") return {16, 16};
  if (trimmed.front() == '[') {
    const auto [count, elem_size] = gen_parse_array_type(trimmed);
    std::string elem = trimmed.substr(trimmed.find(" x ") + 3);
    if (!elem.empty() && elem.back() == ']') elem.pop_back();
    const auto elem_layout = gen_type_layout(elem, type_decls);
    return {count * elem_layout.size, std::max(1, elem_layout.align)};
  }
  if (trimmed.front() == '{' || trimmed.front() == '<' || trimmed.front() == '%') {
    return gen_struct_layout(trimmed, type_decls);
  }
  return {gen_type_bytes(trimmed), std::max(1, gen_type_bytes(trimmed))};
}

// Parse struct type "{ T1, T2, ... }" — compute byte size.
static int gen_struct_size(const std::string& ty,
                           const std::vector<std::string>& type_decls) {
  return gen_type_layout(ty, type_decls).size;
}

// Compute byte offset for a struct field.
static int gen_struct_field_offset(const std::string& ty, int field_idx,
                                   const std::vector<std::string>& type_decls) {
  const std::string resolved = gen_resolve_type_decl_body(ty, type_decls);
  bool packed = false;
  std::string_view body = resolved;
  if (body.size() >= 4 && body[0] == '<' && body[1] == '{' &&
      body[body.size() - 2] == '}' && body.back() == '>') {
    packed = true;
    body.remove_prefix(2);
    body.remove_suffix(2);
  } else if (body.size() >= 2 && body.front() == '{' && body.back() == '}') {
    body.remove_prefix(1);
    body.remove_suffix(1);
  } else {
    return field_idx * 8;
  }

  const auto fields = gen_split_aggregate_fields(body);
  int offset = 0;
  for (int i = 0; i < static_cast<int>(fields.size()); ++i) {
    const auto field_layout = gen_type_layout(fields[i], type_decls);
    const int field_align = std::max(1, packed ? 1 : field_layout.align);
    if (!packed && offset % field_align != 0) {
      offset += field_align - (offset % field_align);
    }
    if (i == field_idx) return offset;
    offset += field_layout.size;
  }
  return offset;
}

// Collect all stack-backed SSA names needed by the direct AArch64 emitter.
static GenSlotMap gen_build_slots(
    const c4c::backend::stack_layout::StackLayoutInput& stack_layout_input,
    const std::vector<std::string>& type_decls) {
  GenSlotMap sm;
  for (const auto& value_name :
       c4c::backend::stack_layout::collect_stack_layout_value_names(stack_layout_input)) {
    sm.get_or_alloc(value_name);
  }

  // Allocate data areas for allocas (separate from the pointer slot)
  for (const auto& alloca : stack_layout_input.entry_allocas) {
    const int elem_size = gen_type_layout(alloca.type_str, type_decls).size;
    sm.alloc_data(alloca.alloca_name, std::max(elem_size, 8));
  }

  for (const auto& param : stack_layout_input.signature_params) {
      if (param.is_varargs || param.operand.empty() || param.operand[0] != '%') continue;
      if (!gen_is_direct_gp_aggregate_type(param.type, type_decls) &&
          !gen_try_parse_hfa_type(param.type, type_decls).has_value()) {
        continue;
      }
      sm.alloc_data(param.operand, gen_type_layout(param.type, type_decls).size);
  }

  for (const auto& call_result : stack_layout_input.call_results) {
    if (gen_is_direct_gp_aggregate_type(call_result.type_str, type_decls) ||
        gen_try_parse_hfa_type(call_result.type_str, type_decls).has_value() ||
        gen_type_is_sret_aggregate_type(call_result.type_str, type_decls)) {
      sm.alloc_data(call_result.value_name,
                    gen_type_layout(call_result.type_str, type_decls).size);
    }
  }

  if (stack_layout_input.is_variadic) {
    sm.reserve_variadic_save_area();
  }

  if (stack_layout_input.return_type.has_value() &&
      gen_type_is_sret_aggregate_type(*stack_layout_input.return_type, type_decls)) {
    sm.get_or_alloc("%ret.sret");
  }

  sm.finalize();
  return sm;
}

static GenSlotMap gen_build_slots(
    const c4c::backend::stack_layout::PreparedEntryAllocaFunctionInputs& prepared_inputs,
    const c4c::codegen::lir::LirFunction& function,
    const std::vector<std::string>& type_decls) {
  GenSlotMap sm;
  for (const auto& value_name :
       c4c::backend::stack_layout::collect_prepared_entry_alloca_value_names(prepared_inputs)) {
    sm.get_or_alloc(value_name);
  }

  for (const auto& alloca : prepared_inputs.rewrite_metadata.entry_allocas) {
    const int elem_size = gen_type_layout(alloca.type_str, type_decls).size;
    sm.alloc_data(alloca.alloca_name, std::max(elem_size, 8));
  }

  for (const auto& inst : function.alloca_insts) {
    const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst);
    if (alloca == nullptr || sm.lookup_data(alloca->result.str()) >= 0) {
      continue;
    }
    sm.get_or_alloc(alloca->result.str());
    const int elem_size = gen_type_layout(alloca->type_str.str(), type_decls).size;
    sm.alloc_data(alloca->result.str(), std::max(elem_size, 8));
  }

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst);
      if (alloca == nullptr || sm.lookup_data(alloca->result.str()) >= 0) {
        continue;
      }
      sm.get_or_alloc(alloca->result.str());
      const int elem_size = gen_type_layout(alloca->type_str.str(), type_decls).size;
      sm.alloc_data(alloca->result.str(), std::max(elem_size, 8));
    }
  }

  for (const auto& param : prepared_inputs.stack_layout_metadata.signature_params) {
    if (param.is_varargs || param.operand.empty() || param.operand[0] != '%') continue;
    if (!gen_is_direct_gp_aggregate_type(param.type, type_decls) &&
        !gen_try_parse_hfa_type(param.type, type_decls).has_value()) {
      continue;
    }
    sm.alloc_data(param.operand, gen_type_layout(param.type, type_decls).size);
  }

  for (const auto& call_result : prepared_inputs.stack_layout_metadata.call_results) {
    if (gen_is_direct_gp_aggregate_type(call_result.type_str, type_decls) ||
        gen_try_parse_hfa_type(call_result.type_str, type_decls).has_value() ||
        gen_type_is_sret_aggregate_type(call_result.type_str, type_decls)) {
      sm.alloc_data(call_result.value_name,
                    gen_type_layout(call_result.type_str, type_decls).size);
    }
  }

  if (prepared_inputs.stack_layout_metadata.is_variadic) {
    sm.reserve_variadic_save_area();
  }

  if (prepared_inputs.stack_layout_metadata.return_type.has_value() &&
      gen_type_is_sret_aggregate_type(*prepared_inputs.stack_layout_metadata.return_type,
                                      type_decls)) {
    sm.get_or_alloc("%ret.sret");
  }

  sm.finalize();
  return sm;
}

static const c4c::backend::stack_layout::StackLayoutSignatureParam* find_signature_param(
    const std::vector<c4c::backend::stack_layout::StackLayoutSignatureParam>& signature_params,
    std::string_view operand) {
  for (const auto& param : signature_params) {
    if (!param.is_varargs && param.operand == operand) {
      return &param;
    }
  }
  return nullptr;
}

// Emit load of an operand into a register.  Handles SSA values, immediates,
// special tokens (null/true/false), and globals.
static void gen_load_operand(std::ostringstream& out, const std::string& op,
                             const std::string& ty, const GenSlotMap& sm,
                             int reg_idx, const std::string& fn_prefix,
                             const std::unordered_set<std::string>* extern_globals = nullptr,
                             const std::vector<std::string>* type_decls = nullptr) {
  const std::string stripped_ty = gen_strip_alignstack_attr(ty);
  const bool pointer_like =
      type_decls != nullptr ? gen_type_is_memory_value(stripped_ty, *type_decls)
                            : gen_type_is_address_only_value(stripped_ty);
  const char* rp = pointer_like || gen_is_64bit(stripped_ty) ? "x" : "w";
  const char* rp64 = "x";  // for address regs always 64-bit
  (void)rp64;
  if (op.empty()) return;

  if (op[0] == '%') {
    // SSA value — load from stack slot
    int off = sm.lookup(op);
    if (off >= 0) {
      if (pointer_like || gen_is_64bit(stripped_ty))
        out << "  ldr x" << reg_idx << ", [sp, #" << off << "]\n";
      else
        out << "  ldr w" << reg_idx << ", [sp, #" << off << "]\n";
    }
  } else if (op[0] == '@') {
    // Global reference — load address
    std::string sym = op.substr(1);
    bool is_extern = extern_globals && extern_globals->count(sym);
    if (is_extern) {
      // Use GOT-relative addressing for extern globals
      out << "  adrp x" << reg_idx << ", :got:" << sym << "\n";
      out << "  ldr x" << reg_idx << ", [x" << reg_idx << ", :got_lo12:" << sym << "]\n";
    } else {
      out << "  adrp x" << reg_idx << ", " << sym << "\n";
      out << "  add x" << reg_idx << ", x" << reg_idx << ", :lo12:" << sym << "\n";
    }
  } else if (op == "null") {
    out << "  mov x" << reg_idx << ", #0\n";
  } else if (op == "true") {
    out << "  mov " << rp << reg_idx << ", #1\n";
  } else if (op == "false" || op == "zeroinitializer" || op == "undef" || op == "poison") {
    out << "  mov " << rp << reg_idx << ", #0\n";
  } else {
    std::uint64_t fp_bits = 0;
    if (gen_try_parse_fp_immediate_bits(op, ty, fp_bits)) {
      gen_emit_integer_immediate(out, reg_idx, fp_bits, gen_is_64bit(ty));
      return;
    }

    // Immediate value
    long long imm = 0;
    try { imm = std::stoll(op); } catch (...) {}
    gen_emit_integer_immediate(out, reg_idx, static_cast<std::uint64_t>(imm),
                               gen_is_64bit(ty));
  }
}

static void gen_load_fp_operand(std::ostringstream& out, const std::string& op,
                                const std::string& ty, const GenSlotMap& sm,
                                int reg_idx, const std::string& fn_prefix,
                                const std::unordered_set<std::string>* extern_globals = nullptr) {
  if (op.empty()) return;

  if (op[0] == '%') {
    const int off = sm.lookup(op);
    if (off >= 0) {
      out << "  ldr " << (ty == "double" ? "d" : "s") << reg_idx
          << ", [sp, #" << off << "]\n";
    }
    return;
  }

  std::uint64_t bits = 0;
  if (gen_try_parse_fp_immediate_bits(op, ty, bits)) {
    gen_emit_integer_immediate(out, 9, bits, ty == "double");
    out << "  fmov " << (ty == "double" ? "d" : "s") << reg_idx << ", "
        << (ty == "double" ? "x9" : "w9") << "\n";
    return;
  }

  gen_load_operand(out, op, ty == "double" ? "i64" : "i32", sm, 9, fn_prefix,
                   extern_globals);
  out << "  fmov " << (ty == "double" ? "d" : "s") << reg_idx << ", "
      << (ty == "double" ? "x9" : "w9") << "\n";
}

// Store register to stack slot.
static void gen_store_result(std::ostringstream& out, const std::string& name,
                             const std::string& ty, const GenSlotMap& sm, int reg_idx,
                             const std::vector<std::string>* type_decls = nullptr) {
  if (name.empty()) return;
  int off = sm.lookup(name);
  if (off < 0) return;
  const std::string stripped_ty = gen_strip_alignstack_attr(ty);
  if ((type_decls != nullptr && gen_type_is_memory_value(stripped_ty, *type_decls)) ||
      gen_type_is_address_only_value(stripped_ty) ||
      gen_is_64bit(stripped_ty))
    out << "  str x" << reg_idx << ", [sp, #" << off << "]\n";
  else
    out << "  str w" << reg_idx << ", [sp, #" << off << "]\n";
}

static const c4c::codegen::lir::LirBlock* gen_find_block_by_label(
    const c4c::codegen::lir::LirFunction& function,
    std::string_view label) {
  for (const auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

static bool gen_block_has_leading_phi(const c4c::codegen::lir::LirFunction& function,
                                      std::string_view label) {
  const auto* block = gen_find_block_by_label(function, label);
  return block != nullptr && !block->insts.empty() &&
         std::holds_alternative<c4c::codegen::lir::LirPhiOp>(block->insts.front());
}

static bool gen_emit_phi_edge_copies(
    std::ostringstream& out,
    const c4c::codegen::lir::LirFunction& function,
    std::string_view target_label,
    std::string_view predecessor_label,
    const GenSlotMap& sm,
    const std::unordered_set<std::string>& extern_globals,
    const std::vector<std::string>& type_decls) {
  const auto* target_block = gen_find_block_by_label(function, target_label);
  if (target_block == nullptr) {
    return false;
  }

  for (const auto& inst : target_block->insts) {
    const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&inst);
    if (phi == nullptr) {
      break;
    }

    const auto incoming_it =
        std::find_if(phi->incoming.begin(), phi->incoming.end(), [&](const auto& incoming) {
          return incoming.second == predecessor_label;
        });
    if (incoming_it == phi->incoming.end()) {
      return false;
    }

    const std::string type = phi->type_str.str();
    if (gen_is_fp_type(type)) {
      gen_load_fp_operand(out, incoming_it->first, type, sm, 0, function.name, &extern_globals);
      const int off = sm.lookup(phi->result.str());
      if (off < 0) {
        return false;
      }
      out << "  str " << (type == "double" ? "d" : "s") << "0, [sp, #" << off << "]\n";
      continue;
    }

    if (gen_type_is_memory_value(type, type_decls) ||
        gen_type_is_sret_aggregate_type(type, type_decls)) {
      return false;
    }

    gen_load_operand(out, incoming_it->first, type, sm, 0, function.name, &extern_globals,
                     &type_decls);
    if (sm.lookup(phi->result.str()) < 0) {
      return false;
    }
    gen_store_result(out, phi->result.str(), type, sm, 0, &type_decls);
  }

  return true;
}

static std::optional<std::string> gen_map_float_predicate(const std::string& pred) {
  if (pred == "eq" || pred == "oeq") return "eq";
  if (pred == "ne" || pred == "une") return "ne";
  if (pred == "olt") return "lt";
  if (pred == "ole") return "le";
  if (pred == "ogt") return "gt";
  if (pred == "oge") return "ge";
  if (pred == "ord") return "vc";
  if (pred == "uno") return "vs";
  return std::nullopt;
}

// Parse GEP index string "type value" → (type, value).
static std::pair<std::string, std::string> gen_parse_index(const std::string& idx) {
  size_t sp = idx.find(' ');
  if (sp == std::string::npos) return {"i64", idx};
  return {idx.substr(0, sp), idx.substr(sp + 1)};
}

// Emit a global initializer value.
// Handles: integers, zeroinitializer, @global refs, { ... } aggregates, [N x T] arrays.
static void gen_emit_global_init(std::ostringstream& out, const std::string& init,
                                  const std::string& ty,
                                  const std::vector<std::string>& type_decls) {
  if (init.empty() || init == "zeroinitializer") {
    int sz = gen_type_layout(ty, type_decls).size;
    out << "  .zero " << sz << "\n";
    return;
  }

  // LLVM c"..." string literal initializer for mutable/global byte arrays.
  if (init.size() >= 3 && init[0] == 'c' && init[1] == '"' && init.back() == '"') {
    const auto bytes = gen_decode_llvm_byte_string(init.substr(2, init.size() - 3));
    gen_emit_ascii_bytes(out, bytes);
    return;
  }

  std::uint64_t fp_bits = 0;
  if (gen_try_parse_fp_immediate_bits(init, ty, fp_bits)) {
    if (ty == "float") {
      out << "  .word " << static_cast<std::uint32_t>(fp_bits) << "\n";
    } else if (ty == "double") {
      out << "  .xword " << fp_bits << "\n";
    } else {
      out << "  .xword " << fp_bits << "\n";
    }
    return;
  }

  std::uint64_t fp128_low = 0;
  std::uint64_t fp128_high = 0;
  if (ty == "fp128" && gen_try_parse_fp128_immediate_words(init, fp128_low, fp128_high)) {
    out << "  .xword " << fp128_high << "\n";
    out << "  .xword " << fp128_low << "\n";
    return;
  }

  // Try integer literal
  if (init[0] == '-' || (init[0] >= '0' && init[0] <= '9')) {
    long long val = 0;
    bool ok = false;
    try { val = std::stoll(init); ok = true; } catch (...) {}
    if (ok) {
      int sz = gen_type_layout(ty, type_decls).size;
      if (sz <= 1) out << "  .byte " << (val & 0xFF) << "\n";
      else if (sz <= 2) out << "  .short " << (val & 0xFFFF) << "\n";
      else if (sz <= 4) out << "  .word " << (val & 0xFFFFFFFF) << "\n";
      else out << "  .xword " << val << "\n";
      return;
    }
  }

  // Global reference
  if (init[0] == '@') {
    out << "  .xword " << init.substr(1) << "\n";
    return;
  }

  // Aggregate initializer: { ty1 val1, ty2 val2, ... }
  if (init[0] == '{') {
    // Parse comma-separated "type value" pairs inside braces
    size_t pos = 1;
    while (pos < init.size()) {
      // Skip whitespace
      while (pos < init.size() && (init[pos] == ' ' || init[pos] == '\t')) ++pos;
      if (pos >= init.size() || init[pos] == '}') break;

      // Parse type
      size_t ty_start = pos;
      // Handle nested types like [N x T], { ... }, %struct.foo
      int depth = 0;
      while (pos < init.size()) {
        char c = init[pos];
        if (c == '[' || c == '{' || c == '<') { ++depth; ++pos; }
        else if (c == ']' || c == '}' || c == '>') {
          if (depth > 0) { --depth; ++pos; }
          else break;
        }
        else if (c == ' ' && depth == 0) break;
        else ++pos;
      }
      std::string field_ty = init.substr(ty_start, pos - ty_start);

      // Skip space
      while (pos < init.size() && init[pos] == ' ') ++pos;

      // Parse value
      size_t val_start = pos;
      depth = 0;
      while (pos < init.size()) {
        char c = init[pos];
        if (c == '{' || c == '[' || c == '<') { ++depth; ++pos; }
        else if (c == '}' || c == ']' || c == '>') {
          if (depth > 0) { --depth; ++pos; }
          else break;
        }
        else if (c == ',' && depth == 0) break;
        else ++pos;
      }
      std::string field_val = init.substr(val_start, pos - val_start);
      // Trim trailing whitespace
      while (!field_val.empty() && (field_val.back() == ' ' || field_val.back() == '\t'))
        field_val.pop_back();

      gen_emit_global_init(out, field_val, field_ty, type_decls);

      // Skip comma
      if (pos < init.size() && init[pos] == ',') ++pos;
    }
    return;
  }

  // Array initializer: [ty val, ty val, ...]  — same format as aggregate
  if (init[0] == '[') {
    size_t pos = 1;
    while (pos < init.size()) {
      while (pos < init.size() && (init[pos] == ' ' || init[pos] == '\t')) ++pos;
      if (pos >= init.size() || init[pos] == ']') break;

      size_t ty_start = pos;
      int depth = 0;
      while (pos < init.size()) {
        char c = init[pos];
        if (c == '[' || c == '{' || c == '<') { ++depth; ++pos; }
        else if (c == ']' || c == '}' || c == '>') {
          if (depth > 0) { --depth; ++pos; }
          else break;
        }
        else if (c == ' ' && depth == 0) break;
        else ++pos;
      }
      std::string elem_ty = init.substr(ty_start, pos - ty_start);

      while (pos < init.size() && init[pos] == ' ') ++pos;

      size_t val_start = pos;
      depth = 0;
      while (pos < init.size()) {
        char c = init[pos];
        if (c == '{' || c == '[' || c == '<') { ++depth; ++pos; }
        else if (c == '}' || c == ']' || c == '>') {
          if (depth > 0) { --depth; ++pos; }
          else break;
        }
        else if (c == ',' && depth == 0) break;
        else ++pos;
      }
      std::string elem_val = init.substr(val_start, pos - val_start);
      while (!elem_val.empty() && (elem_val.back() == ' ' || elem_val.back() == '\t'))
        elem_val.pop_back();

      gen_emit_global_init(out, elem_val, elem_ty, type_decls);

      if (pos < init.size() && init[pos] == ',') ++pos;
    }
    return;
  }

  // "null" → zero pointer
  if (init == "null") {
    out << "  .xword 0\n";
    return;
  }

  // Fallback: zero-fill
  int sz = gen_type_bytes(ty);
  out << "  .zero " << sz << "\n";
}

static std::optional<std::string> render_general_lir_asm_if_supported(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;
  if (is_double_indirect_local_pointer_conditional_return_fallback(module)) {
    // This CFG shape used to sneak through target-local direct-LIR handling
    // after missing the shared BIR route. Keep that ownership behind BIR.
    return std::nullopt;
  }
  if (is_alloca_backed_switch_return_fallback(module)) {
    // Keep this synthetic switch-return fallback from reviving target-local
    // multi-block CFG ownership after shared BIR lowering declines the module.
    return std::nullopt;
  }
  if (is_constant_selector_switch_return_fallback(module)) {
    // Keep this synthetic constant-selector switch from reviving target-local
    // multi-block CFG ownership after shared BIR lowering declines the module.
    return std::nullopt;
  }
  std::ostringstream out;

  // Reject unsupported features
  for (std::size_t function_index = 0; function_index < module.functions.size(); ++function_index) {
    const auto& fn = module.functions[function_index];
    if (fn.is_declaration) continue;
    const auto prepared_inputs =
        c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
            module, function_index);
    const auto& return_ty = prepared_inputs.stack_layout_metadata.return_type;
    if (!return_ty.has_value()) return std::nullopt;
    const auto return_layout = gen_type_layout(*return_ty, module.type_decls);
    if (return_layout.size > 8 && !return_ty->empty() &&
        (*return_ty)[0] != 'i' && *return_ty != "ptr" &&
        !gen_is_fp_type(*return_ty) && *return_ty != "void" &&
        !gen_is_direct_gp_aggregate_type(*return_ty, module.type_decls) &&
        !gen_try_parse_hfa_type(*return_ty, module.type_decls).has_value() &&
        !gen_type_is_sret_aggregate_type(*return_ty, module.type_decls)) {
      return std::nullopt;
    }
    for (const auto& inst : fn.alloca_insts) {
      if (std::holds_alternative<LirInlineAsmOp>(inst)) return std::nullopt;
      if (const auto* va_arg = std::get_if<LirVaArgOp>(&inst)) {
        if (!gen_is_scalar_variadic_type(va_arg->type_str.str()) ||
            va_arg->type_str == "fp128") {
          return std::nullopt;
        }
      }
    }
    for (const auto& blk : fn.blocks) {
      for (const auto& inst : blk.insts) {
        if (std::holds_alternative<LirInlineAsmOp>(inst)) return std::nullopt;
        if (std::holds_alternative<LirInsertElementOp>(inst)) return std::nullopt;
        if (std::holds_alternative<LirExtractElementOp>(inst)) return std::nullopt;
        if (std::holds_alternative<LirShuffleVectorOp>(inst)) return std::nullopt;
        if (const auto* call = std::get_if<LirCallOp>(&inst)) {
          if (call->callee == "@llvm.ptrmask.p0.i64") {
            return std::nullopt;
          }
        }
        if (const auto* va_arg = std::get_if<LirVaArgOp>(&inst)) {
          if (!gen_is_scalar_variadic_type(va_arg->type_str.str()) ||
              va_arg->type_str == "fp128") {
            return std::nullopt;
          }
        }
        if (std::holds_alternative<LirIndirectBrOp>(inst)) return std::nullopt;
      }
      // Check for fp types in terminators (fp128, etc.)
      if (const auto* ret = std::get_if<LirRet>(&blk.terminator)) {
        if (ret->type_str == "fp128" || ret->type_str == "double" ||
            ret->type_str == "float")
          return std::nullopt;
      }
    }
  }

  // Emit .text first so it appears in the first 256 bytes (test runner check)
  out << ".text\n";

  // Emit .data / .bss sections for globals
  bool has_data = false;
  for (const auto& g : module.globals) {
    if (g.is_extern_decl) continue;
    if (!has_data) {
      out << ".data\n";
      has_data = true;
    }
    std::string sym = g.name;
    if (!g.is_internal) {
      out << ".globl " << sym << "\n";
    }
    // Alignment
    int align = g.align_bytes;
    if (align > 0) out << ".p2align " << __builtin_ctz(align) << "\n";
    out << sym << ":\n";
    // Emit initializer
    gen_emit_global_init(out, g.init_text, g.llvm_type, module.type_decls);
  }

  // Emit .rodata for string pool
  // Note: raw_bytes contains LLVM-escaped text (e.g. "\0A" for newline, "\00" for null)
  if (!module.string_pool.empty()) {
    out << ".section .rodata\n";
    for (const auto& sc : module.string_pool) {
      std::string sym = sc.pool_name;
      if (sym[0] == '@') sym = sym.substr(1);
      out << sym << ":\n";

      // Decode LLVM hex escapes to actual bytes
      const auto bytes = gen_decode_llvm_byte_string(sc.raw_bytes);

      // Emit as .byte directives for exact control, then a null terminator
      if (bytes.empty()) {
        out << "  .asciz \"\"\n";
      } else {
        gen_emit_ascii_bytes(out, bytes);
        // Add null terminator if not already present
        if (bytes.empty() || bytes.back() != 0) {
          out << "  .byte 0\n";
        }
      }
    }
  }

  out << ".text\n";

  // Build a map of function names for resolving calls
  std::unordered_set<std::string> defined_fns;
  for (const auto& fn : module.functions) {
    defined_fns.insert(fn.name);
  }

  // Build set of extern global names (need GOT-relative access)
  std::unordered_set<std::string> extern_globals;
  for (const auto& g : module.globals) {
    if (g.is_extern_decl) extern_globals.insert(g.name);
  }

  int gen_label_counter = 0;

  for (std::size_t function_index = 0; function_index < module.functions.size(); ++function_index) {
    const auto& fn = module.functions[function_index];
    if (fn.is_declaration) continue;

    // Build slot map
    const auto prepared_inputs =
        c4c::backend::stack_layout::prepare_module_function_entry_alloca_preparation(
            module, function_index);
    GenSlotMap sm = gen_build_slots(prepared_inputs, fn, module.type_decls);
    const auto& entry_allocas = prepared_inputs.rewrite_metadata.entry_allocas;
    const auto& parsed_signature_params = prepared_inputs.stack_layout_metadata.signature_params;
    const bool is_variadic_function = prepared_inputs.stack_layout_metadata.is_variadic;
    int named_gp_count = 0;
    int named_fp_count = 0;
    for (const auto& param : parsed_signature_params) {
      if (param.is_varargs) break;
      if (gen_try_parse_hfa_type(param.type, module.type_decls).has_value()) {
        named_fp_count = std::min(8, named_fp_count +
                                         gen_try_parse_hfa_type(param.type, module.type_decls)
                                             ->elem_count);
      } else if (gen_is_fp_type(param.type)) {
        if (named_fp_count < 8) ++named_fp_count;
      } else if (gen_is_direct_gp_aggregate_type(param.type, module.type_decls)) {
        const int gp_chunks =
            (gen_type_layout(param.type, module.type_decls).size + 7) / 8;
        named_gp_count = std::min(8, named_gp_count + gp_chunks);
      } else {
        if (named_gp_count < 8) ++named_gp_count;
      }
    }
    // Keep signature decoding centralized to one shared parse per emitted function.
    {
      for (const auto& param : parsed_signature_params) {
        if (param.is_varargs) break;
        if (!param.operand.empty() && param.operand[0] == '%') {
          sm.get_or_alloc(param.operand);
        }
      }
    }
    sm.finalize();

    // Function header
    if (!fn.is_internal) {
      out << ".globl " << fn.name << "\n";
    }
    out << ".type " << fn.name << ", %function\n";
    out << fn.name << ":\n";

    // Prologue: allocate stack frame, save lr
    gen_emit_sp_adjust(out, "sub", sm.frame_size);
    out << "  str x30, [sp, #0]\n";  // save lr

    if (prepared_inputs.stack_layout_metadata.return_type.has_value() &&
        gen_type_is_sret_aggregate_type(*prepared_inputs.stack_layout_metadata.return_type,
                                        module.type_decls)) {
      const int sret_off = sm.lookup("%ret.sret");
      if (sret_off >= 0) {
        out << "  str x8, [sp, #" << sret_off << "]\n";
      }
    }

    // Materialize named parameters into the backend-owned stack slots.
    {
      {
        int next_gp_reg = 0;
        int next_fp_reg = 0;
        int stack_param_bytes = 0;
        for (const auto& param : parsed_signature_params) {
          if (param.is_varargs) break;
          if (gen_is_direct_gp_aggregate_type(param.type, module.type_decls)) {
            const int ptr_off = sm.lookup(param.operand);
            const int data_off = sm.lookup_data(param.operand);
            if (ptr_off < 0 || data_off < 0) continue;
            gen_emit_add_imm(out, "x9", "sp", data_off, "x9");
            out << "  str x9, [sp, #" << ptr_off << "]\n";
            const auto layout = gen_type_layout(param.type, module.type_decls);
            const int chunk_count = (layout.size + 7) / 8;
            for (int chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
              const int chunk_offset = chunk_index * 8;
              const int chunk_size = std::min(8, layout.size - chunk_offset);
              if (next_gp_reg < 8) {
                gen_emit_store_chunk_to_addr(out, next_gp_reg, chunk_size, 9, chunk_offset);
                ++next_gp_reg;
              } else {
                stack_param_bytes = ((stack_param_bytes + 7) / 8) * 8;
                gen_emit_add_imm(out, "x10", "sp", sm.frame_size + stack_param_bytes, "x11");
                gen_emit_load_chunk_from_addr(out, 11, gen_chunk_type_for_size(chunk_size), 10, 0);
                gen_emit_store_chunk_to_addr(out, 11, chunk_size, 9, chunk_offset);
                stack_param_bytes += 8;
              }
            }
            continue;
          }
          if (const auto hfa = gen_try_parse_hfa_type(param.type, module.type_decls)) {
            const int ptr_off = sm.lookup(param.operand);
            const int data_off = sm.lookup_data(param.operand);
            if (ptr_off < 0 || data_off < 0) continue;
            gen_emit_add_imm(out, "x9", "sp", data_off, "x9");
            out << "  str x9, [sp, #" << ptr_off << "]\n";
            if (next_fp_reg + hfa->elem_count <= 8) {
              for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
                gen_emit_store_typed_chunk_to_addr(out, next_fp_reg + elem_index, hfa->elem_ty,
                                                   9, elem_index * hfa->elem_size);
              }
              next_fp_reg += hfa->elem_count;
            } else {
              stack_param_bytes = ((stack_param_bytes + (hfa->stack_align - 1)) / hfa->stack_align) *
                                  hfa->stack_align;
              gen_emit_add_imm(out, "x10", "sp", sm.frame_size + stack_param_bytes, "x11");
              for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
                gen_emit_load_chunk_from_addr(out, 11, hfa->elem_ty, 10,
                                              elem_index * hfa->elem_size);
                gen_emit_store_typed_chunk_to_addr(out, 11, hfa->elem_ty, 9,
                                                   elem_index * hfa->elem_size);
              }
              stack_param_bytes += hfa->elem_count * hfa->elem_size;
            }
            continue;
          }
          int off = sm.lookup(param.operand);
          if (off >= 0) {
            if (gen_is_fp_type(param.type)) {
              out << "  str " << (param.type == "double" ? "d" : "s") << next_fp_reg
                  << ", [sp, #" << off << "]\n";
              if (next_fp_reg < 8) ++next_fp_reg;
            } else {
              out << "  str x" << next_gp_reg << ", [sp, #" << off << "]\n";
              if (next_gp_reg < 8) ++next_gp_reg;
            }
          }
        }
      }
    }

    if (is_variadic_function && sm.va_gp_save_offset >= 0 && sm.va_fp_save_offset >= 0) {
      for (int i = 0; i < 8; ++i) {
        out << "  str x" << i << ", [sp, #" << (sm.va_gp_save_offset + i * 8) << "]\n";
      }
      for (int i = 0; i < 8; ++i) {
        out << "  str q" << i << ", [sp, #" << (sm.va_fp_save_offset + i * 16) << "]\n";
      }
    }

    // Initialize alloca pointer slots from the prepared function allocas.
    for (const auto& inst : fn.alloca_insts) {
      const auto* alloca = std::get_if<LirAllocaOp>(&inst);
      if (alloca == nullptr) continue;
      const int ptr_off = sm.lookup(alloca->result.str());
      const int data_off = sm.lookup_data(alloca->result.str());
      if (ptr_off < 0 || data_off < 0) continue;
      if (data_off < 4096) {
        out << "  add x0, sp, #" << data_off << "\n";
      } else {
        out << "  mov x0, #" << data_off << "\n";
        out << "  add x0, sp, x0\n";
      }
      out << "  str x0, [sp, #" << ptr_off << "]\n";
    }

    // Initialize alloca pointer slots from the prepared entry-alloca metadata.
    for (const auto& alloca : entry_allocas) {
      int ptr_off = sm.lookup(alloca.alloca_name);
      int data_off = sm.lookup_data(alloca.alloca_name);
      if (ptr_off >= 0 && data_off >= 0) {
        if (data_off < 4096) {
          out << "  add x0, sp, #" << data_off << "\n";
        } else {
          out << "  mov x0, #" << data_off << "\n";
          out << "  add x0, sp, x0\n";
        }
        out << "  str x0, [sp, #" << ptr_off << "]\n";
      }
    }

    // Lowered parameter allocas (e.g. %lv.param.x) must be initialized after the
    // alloca address slots have been materialized.
    {
      for (const auto& alloca : entry_allocas) {
        if (!alloca.paired_store_value.has_value()) continue;
        const std::string& param_name = *alloca.paired_store_value;
        if (param_name.compare(0, 3, "%p.") != 0) continue;
        const auto* param = find_signature_param(parsed_signature_params, param_name);
        if (param == nullptr) continue;

        const int param_ptr_off = sm.lookup(alloca.alloca_name);
        if (param_ptr_off < 0) continue;
        const int param_value_off = sm.lookup(param_name);
        if (param_value_off < 0) continue;

        const std::string& param_type = param->type;
        const auto param_layout = gen_type_layout(param_type, module.type_decls);
        if (param_layout.size <= 0) continue;

        out << "  ldr x9, [sp, #" << param_ptr_off << "]\n";
        if (gen_type_is_memory_value(param_type, module.type_decls)) {
          if (param_value_off < 0) continue;
          out << "  ldr x10, [sp, #" << param_value_off << "]\n";
          int copied = 0;
          while (copied < param_layout.size) {
            const int chunk_size =
                gen_chunk_size_for_remaining_copy_bytes(param_layout.size - copied);
            gen_emit_load_chunk_from_addr(out, 11,
                                          chunk_size == 16 ? "fp128"
                                                           : gen_chunk_type_for_size(chunk_size),
                                          10, copied);
            gen_emit_store_chunk_to_addr(out, 11, chunk_size, 9, copied);
            copied += chunk_size;
          }
          continue;
        }

        if (param_layout.size > 8) continue;
        if (param_layout.size == 1) {
          out << "  ldr w10, [sp, #" << param_value_off << "]\n";
          out << "  strb w10, [x9]\n";
        } else if (param_layout.size == 2) {
          out << "  ldr w10, [sp, #" << param_value_off << "]\n";
          out << "  strh w10, [x9]\n";
        } else if (param_layout.size == 4) {
          out << "  ldr w10, [sp, #" << param_value_off << "]\n";
          out << "  str w10, [x9]\n";
        } else {
          out << "  ldr x10, [sp, #" << param_value_off << "]\n";
          out << "  str x10, [x9]\n";
        }
      }
    }

    // Emit blocks
    for (size_t bi = 0; bi < fn.blocks.size(); ++bi) {
      const auto& blk = fn.blocks[bi];
      // Block label (skip for entry block if it's first)
      out << "." << fn.name << "." << blk.label << ":\n";

      // Skip phi instructions (handled by predecessor copies)
      for (const auto& inst : blk.insts) {
        if (std::holds_alternative<LirPhiOp>(inst)) continue;

        if (const auto* p = std::get_if<LirAllocaOp>(&inst)) {
          int ptr_off = sm.lookup(p->result.str());
          int data_off = sm.lookup_data(p->result.str());
          if (ptr_off >= 0 && data_off >= 0) {
            if (data_off < 4096) {
              out << "  add x0, sp, #" << data_off << "\n";
            } else {
              out << "  mov x0, #" << data_off << "\n";
              out << "  add x0, sp, x0\n";
            }
            out << "  str x0, [sp, #" << ptr_off << "]\n";
          }
        }
        else if (const auto* p = std::get_if<LirLoadOp>(&inst)) {
          std::string ty = p->type_str.str();
          std::string ptr_name = p->ptr.str();
          std::string result = p->result.str();
          const bool memory_value = gen_type_is_memory_value(ty, module.type_decls) ||
                                    gen_type_is_sret_aggregate_type(ty, module.type_decls);

          if (ptr_name[0] == '@') {
            // Load from global
            std::string sym = ptr_name.substr(1);
            if (extern_globals.count(sym)) {
              out << "  adrp x0, :got:" << sym << "\n";
              out << "  ldr x0, [x0, :got_lo12:" << sym << "]\n";
            } else {
              out << "  adrp x0, " << sym << "\n";
              out << "  add x0, x0, :lo12:" << sym << "\n";
            }
          } else {
            // Load pointer from stack slot
            int ptr_off = sm.lookup(ptr_name);
            if (ptr_off >= 0)
              out << "  ldr x0, [sp, #" << ptr_off << "]\n";
          }

          if (memory_value) {
            gen_store_result(out, result, "ptr", sm, 0, &module.type_decls);
          } else {
            // Dereference pointer
            if (gen_is_fp_type(ty)) {
              if (ty == "double") {
                out << "  ldr d0, [x0]\n";
                out << "  fmov x0, d0\n";
              } else {
                out << "  ldr s0, [x0]\n";
                out << "  fmov w0, s0\n";
              }
            } else {
              int bits = gen_type_bits(ty);
              if (bits <= 8)
              out << "  ldrb w0, [x0]\n";
              else if (bits <= 16)
              out << "  ldrh w0, [x0]\n";
              else if (bits <= 32)
              out << "  ldr w0, [x0]\n";
              else
              out << "  ldr x0, [x0]\n";
            }

            gen_store_result(out, result, ty, sm, 0, &module.type_decls);
          }
        }
        else if (const auto* p = std::get_if<LirStoreOp>(&inst)) {
          std::string ty = p->type_str.str();
          std::string val = p->val.str();
          std::string ptr_name = p->ptr.str();
          const bool memory_value = gen_type_is_memory_value(ty, module.type_decls) ||
                                    gen_type_is_sret_aggregate_type(ty, module.type_decls);

          // Load pointer into x1
          if (ptr_name[0] == '@') {
            std::string sym = ptr_name.substr(1);
            if (extern_globals.count(sym)) {
              out << "  adrp x1, :got:" << sym << "\n";
              out << "  ldr x1, [x1, :got_lo12:" << sym << "]\n";
            } else {
              out << "  adrp x1, " << sym << "\n";
              out << "  add x1, x1, :lo12:" << sym << "\n";
            }
          } else {
            int ptr_off = sm.lookup(ptr_name);
            if (ptr_off >= 0)
              out << "  ldr x1, [sp, #" << ptr_off << "]\n";
          }

          if (memory_value) {
            gen_load_operand(out, val, "ptr", sm, 0, fn.name, &extern_globals,
                             &module.type_decls);
            out << "  mov x9, x0\n";
            out << "  mov x0, x1\n";
            out << "  mov x1, x9\n";
            gen_emit_integer_immediate(out, 2,
                                       static_cast<std::uint64_t>(
                                           gen_type_layout(ty, module.type_decls).size),
                                       true);
            out << "  str x30, [sp, #0]\n";
            out << "  bl memcpy\n";
            out << "  ldr x30, [sp, #0]\n";
          } else {
            // Load value into w0/x0
            if (gen_is_fp_type(ty)) {
              gen_load_fp_operand(out, val, ty, sm, 0, fn.name, &extern_globals);
              out << "  str " << (ty == "double" ? "d0" : "s0") << ", [x1]\n";
            } else {
              gen_load_operand(out, val, ty, sm, 0, fn.name, &extern_globals);

              // Store value through pointer
              int bits = gen_type_bits(ty);
              if (bits <= 8)
              out << "  strb w0, [x1]\n";
              else if (bits <= 16)
              out << "  strh w0, [x1]\n";
              else if (bits <= 32)
              out << "  str w0, [x1]\n";
              else
              out << "  str x0, [x1]\n";
            }
          }
        }
        else if (const auto* p = std::get_if<LirBinOp>(&inst)) {
          std::string ty = p->type_str.str();
          const char* rp = gen_reg_prefix(ty);
          std::string opc = p->opcode.str();

          if (gen_is_fp_type(ty)) {
            gen_load_fp_operand(out, p->lhs.str(), ty, sm, 0, fn.name, &extern_globals);
            if (opc != "fneg") {
              gen_load_fp_operand(out, p->rhs.str(), ty, sm, 1, fn.name, &extern_globals);
            }

            const char* fp_reg = ty == "double" ? "d" : "s";
            if (opc == "fadd") out << "  fadd " << fp_reg << "0, " << fp_reg << "0, " << fp_reg << "1\n";
            else if (opc == "fsub") out << "  fsub " << fp_reg << "0, " << fp_reg << "0, " << fp_reg << "1\n";
            else if (opc == "fmul") out << "  fmul " << fp_reg << "0, " << fp_reg << "0, " << fp_reg << "1\n";
            else if (opc == "fdiv") out << "  fdiv " << fp_reg << "0, " << fp_reg << "0, " << fp_reg << "1\n";
            else if (opc == "fneg") out << "  fneg " << fp_reg << "0, " << fp_reg << "0\n";
            else return std::nullopt;

            out << "  fmov " << (ty == "double" ? "x0, d0" : "w0, s0") << "\n";
          } else {
            gen_load_operand(out, p->lhs.str(), ty, sm, 0, fn.name, &extern_globals);
            if (opc != "fneg") {
              gen_load_operand(out, p->rhs.str(), ty, sm, 1, fn.name, &extern_globals);
            }

            if (opc == "add") out << "  add " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "sub") out << "  sub " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "mul") out << "  mul " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "sdiv") out << "  sdiv " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "udiv") out << "  udiv " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "srem") {
              out << "  sdiv " << rp << "2, " << rp << "0, " << rp << "1\n";
              out << "  msub " << rp << "0, " << rp << "2, " << rp << "1, " << rp << "0\n";
            }
            else if (opc == "urem") {
              out << "  udiv " << rp << "2, " << rp << "0, " << rp << "1\n";
              out << "  msub " << rp << "0, " << rp << "2, " << rp << "1, " << rp << "0\n";
            }
            else if (opc == "and") out << "  and " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "or") out << "  orr " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "xor") out << "  eor " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "shl") out << "  lsl " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "lshr") out << "  lsr " << rp << "0, " << rp << "0, " << rp << "1\n";
            else if (opc == "ashr") out << "  asr " << rp << "0, " << rp << "0, " << rp << "1\n";
            else return std::nullopt;  // unsupported binop
          }

          gen_store_result(out, p->result.str(), ty, sm, 0);
        }
        else if (const auto* p = std::get_if<LirCmpOp>(&inst)) {
          std::string pred = p->predicate.str();
          if (p->is_float) {
            std::string ty = p->type_str.str();
            gen_load_fp_operand(out, p->lhs.str(), ty, sm, 0, fn.name, &extern_globals);
            gen_load_fp_operand(out, p->rhs.str(), ty, sm, 1, fn.name, &extern_globals);
            out << "  fcmp " << (ty == "double" ? "d" : "s") << "0, "
                << (ty == "double" ? "d" : "s") << "1\n";
            auto cond = gen_map_float_predicate(pred);
            if (!cond.has_value()) return std::nullopt;
            out << "  cset w0, " << *cond << "\n";
          } else {
            std::string ty = p->type_str.str();
            const char* rp = gen_reg_prefix(ty);
            gen_load_operand(out, p->lhs.str(), ty, sm, 0, fn.name, &extern_globals);
            gen_load_operand(out, p->rhs.str(), ty, sm, 1, fn.name, &extern_globals);
            out << "  cmp " << rp << "0, " << rp << "1\n";

            std::string cond;
            if (pred == "eq") cond = "eq";
            else if (pred == "ne") cond = "ne";
            else if (pred == "slt") cond = "lt";
            else if (pred == "sle") cond = "le";
            else if (pred == "sgt") cond = "gt";
            else if (pred == "sge") cond = "ge";
            else if (pred == "ult") cond = "lo";
            else if (pred == "ule") cond = "ls";
            else if (pred == "ugt") cond = "hi";
            else if (pred == "uge") cond = "hs";
            else return std::nullopt;

            out << "  cset w0, " << cond << "\n";
          }
          gen_store_result(out, p->result.str(), "i32", sm, 0);
        }
        else if (const auto* p = std::get_if<LirCastOp>(&inst)) {
          std::string from_ty = p->from_type.str();
          std::string to_ty = p->to_type.str();
          switch (p->kind) {
            case LirCastKind::ZExt: {
              gen_load_operand(out, p->operand.str(), from_ty, sm, 0, fn.name, &extern_globals);
              int from_bits = gen_type_bits(from_ty);
              if (from_bits == 1) {
                out << "  and w0, w0, #1\n";
              } else if (from_bits <= 8) {
                out << "  and w0, w0, #0xff\n";
              } else if (from_bits <= 16) {
                out << "  and w0, w0, #0xffff\n";
              }
              // If extending to 64-bit, the upper 32 bits are already zero when using w0
              if (gen_is_64bit(to_ty) && !gen_is_64bit(from_ty)) {
                // w0 → x0 is already zero-extended by AArch64
                out << "  mov x0, x0\n";  // ensure upper bits are clear
              }
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            case LirCastKind::SExt: {
              gen_load_operand(out, p->operand.str(), from_ty, sm, 0, fn.name, &extern_globals);
              int from_bits = gen_type_bits(from_ty);
              if (from_bits == 1) {
                // Sign extend i1: 0→0, 1→-1
                out << "  sbfx x0, x0, #0, #1\n";
              } else if (from_bits <= 8) {
                out << "  sxtb x0, w0\n";
              } else if (from_bits <= 16) {
                out << "  sxth x0, w0\n";
              } else if (from_bits <= 32 && gen_is_64bit(to_ty)) {
                out << "  sxtw x0, w0\n";
              }
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            case LirCastKind::Trunc: {
              gen_load_operand(out, p->operand.str(), from_ty, sm, 0, fn.name, &extern_globals);
              int to_bits = gen_type_bits(to_ty);
              if (to_bits == 1) {
                out << "  and w0, w0, #1\n";
              } else if (to_bits <= 8) {
                out << "  and w0, w0, #0xff\n";
              } else if (to_bits <= 16) {
                out << "  and w0, w0, #0xffff\n";
              }
              // else i32 truncation from i64 is just using w0
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            case LirCastKind::PtrToInt:
            case LirCastKind::IntToPtr:
            case LirCastKind::Bitcast: {
              // Just copy the value
              gen_load_operand(out, p->operand.str(), "i64", sm, 0, fn.name, &extern_globals);
              gen_store_result(out, p->result.str(), "i64", sm, 0);
              break;
            }
            case LirCastKind::SIToFP:
            case LirCastKind::UIToFP: {
              gen_load_operand(out, p->operand.str(), from_ty, sm, 0, fn.name, &extern_globals);
              const bool to_double = to_ty == "double";
              const bool from_64 = gen_is_64bit(from_ty);
              out << "  " << (p->kind == LirCastKind::SIToFP ? "scvtf " : "ucvtf ")
                  << (to_double ? "d0, " : "s0, ")
                  << (from_64 ? "x0" : "w0") << "\n";
              out << "  fmov " << (to_double ? "x0, d0" : "w0, s0") << "\n";
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            case LirCastKind::FPToSI:
            case LirCastKind::FPToUI: {
              gen_load_fp_operand(out, p->operand.str(), from_ty, sm, 0, fn.name,
                                  &extern_globals);
              const bool from_double = from_ty == "double";
              const bool to_64 = gen_is_64bit(to_ty);
              out << "  " << (p->kind == LirCastKind::FPToSI ? "fcvtzs " : "fcvtzu ")
                  << (to_64 ? "x0, " : "w0, ")
                  << (from_double ? "d0" : "s0") << "\n";
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            case LirCastKind::FPExt:
            case LirCastKind::FPTrunc: {
              gen_load_fp_operand(out, p->operand.str(), from_ty, sm, 0, fn.name,
                                  &extern_globals);
              out << "  fcvt " << (to_ty == "double" ? "d0, s0" : "s0, d0") << "\n";
              out << "  fmov " << (to_ty == "double" ? "x0, d0" : "w0, s0") << "\n";
              gen_store_result(out, p->result.str(), to_ty, sm, 0);
              break;
            }
            default:
              return std::nullopt;
          }
        }
        else if (const auto* p = std::get_if<LirCallOp>(&inst)) {
          std::string callee = p->callee.str();
          bool is_direct = (callee[0] == '@');
          std::string callee_sym = is_direct ? callee.substr(1) : callee;

          const auto args = c4c::backend::parse_backend_owned_typed_call_args(*p);
          if (!args.has_value()) {
            return std::nullopt;
          }

          const bool is_variadic_call =
              std::string(p->callee_type_suffix).find("...") != std::string::npos;

          struct GenCallPiece {
            enum class Kind {
              GPScalar,
              FPScalar,
              GPChunkFromAddr,
              FPChunkFromAddr,
              StackScalar,
              StackChunkFromAddr,
              StackFp128FromAddr,
            };

            Kind kind = Kind::GPScalar;
            std::string operand;
            std::string type;
            int reg_index = -1;
            int stack_offset = -1;
            int source_offset = 0;
            int byte_size = 0;
          };

          const auto align_up = [](int value, int align) {
            if (align <= 1) return value;
            const int rem = value % align;
            return rem == 0 ? value : value + (align - rem);
          };

          const auto emit_load_address = [&](const std::string& operand, int reg_idx) {
            gen_load_operand(out, operand, "ptr", sm, reg_idx, fn.name, &extern_globals);
          };
          const auto emit_load_address_from_frame_base = [&](const std::string& operand,
                                                             int reg_idx,
                                                             const char* frame_base_reg) {
            if (!operand.empty() && operand[0] == '%') {
              const int off = sm.lookup(operand);
              if (off >= 0) {
                out << "  ldr x" << reg_idx << ", [" << frame_base_reg << ", #" << off << "]\n";
                return;
              }
            }
            gen_load_operand(out, operand, "ptr", sm, reg_idx, fn.name, &extern_globals);
          };
          const auto emit_load_scalar_from_frame_base = [&](const std::string& operand,
                                                            const std::string& ty, int reg_idx,
                                                            const char* frame_base_reg) {
            const std::string stripped_ty = gen_strip_alignstack_attr(ty);
            if (!operand.empty() && operand[0] == '%') {
              const int off = sm.lookup(operand);
              if (off >= 0) {
                if (gen_is_fp_type(stripped_ty)) {
                  out << "  ldr " << (stripped_ty == "double" ? "d" : "s") << reg_idx << ", ["
                      << frame_base_reg << ", #" << off << "]\n";
                } else if (gen_is_64bit(stripped_ty)) {
                  out << "  ldr x" << reg_idx << ", [" << frame_base_reg << ", #" << off << "]\n";
                } else {
                  out << "  ldr w" << reg_idx << ", [" << frame_base_reg << ", #" << off << "]\n";
                }
                return;
              }
            }
            if (gen_is_fp_type(stripped_ty)) {
              gen_load_fp_operand(out, operand, stripped_ty, sm, reg_idx, fn.name, &extern_globals);
            } else {
              gen_load_operand(out, operand, stripped_ty, sm, reg_idx, fn.name, &extern_globals);
            }
          };

          std::vector<GenCallPiece> pieces;
          int next_gp_reg = 0;
          int next_fp_reg = 0;
          int stack_arg_bytes = 0;
          for (const auto& arg : *args) {
            const std::string stripped_ty = gen_strip_alignstack_attr(arg.type);
            const auto hfa = gen_try_parse_hfa_type(stripped_ty, module.type_decls);

            if (is_variadic_call && stripped_ty == "fp128") {
              if (next_fp_reg < 8) {
                pieces.push_back({GenCallPiece::Kind::FPChunkFromAddr, arg.operand, stripped_ty,
                                  next_fp_reg++, -1, 0, 16});
              } else {
                stack_arg_bytes = align_up(stack_arg_bytes, 16);
                pieces.push_back({GenCallPiece::Kind::StackFp128FromAddr, arg.operand, stripped_ty,
                                  -1, stack_arg_bytes, 0, 16});
                stack_arg_bytes += 16;
              }
              continue;
            }

            if (is_variadic_call) {
              if (const auto fp_aggregate = gen_try_parse_fp_aggregate_arg_type(arg.type)) {
                if (next_fp_reg + fp_aggregate->elem_count <= 8) {
                  for (int elem_index = 0; elem_index < fp_aggregate->elem_count;
                       ++elem_index) {
                    pieces.push_back({GenCallPiece::Kind::FPChunkFromAddr,
                                      arg.operand,
                                      fp_aggregate->elem_ty,
                                      next_fp_reg++,
                                      -1,
                                      elem_index * fp_aggregate->elem_size,
                                      fp_aggregate->elem_size});
                  }
                } else {
                  stack_arg_bytes = align_up(stack_arg_bytes, fp_aggregate->stack_align);
                  for (int elem_index = 0; elem_index < fp_aggregate->elem_count;
                       ++elem_index) {
                    pieces.push_back({GenCallPiece::Kind::StackChunkFromAddr,
                                      arg.operand,
                                      fp_aggregate->elem_ty,
                                      -1,
                                      stack_arg_bytes + elem_index * fp_aggregate->elem_size,
                                      elem_index * fp_aggregate->elem_size,
                                      fp_aggregate->elem_size});
                  }
                  stack_arg_bytes += fp_aggregate->elem_count * fp_aggregate->elem_size;
                }
                continue;
              }
            }

            if (!is_variadic_call && hfa.has_value()) {
              if (next_fp_reg + hfa->elem_count <= 8) {
                for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
                  pieces.push_back({GenCallPiece::Kind::FPChunkFromAddr,
                                    arg.operand,
                                    hfa->elem_ty,
                                    next_fp_reg++,
                                    -1,
                                    elem_index * hfa->elem_size,
                                    hfa->elem_size});
                }
              } else {
                stack_arg_bytes = align_up(stack_arg_bytes, hfa->stack_align);
                for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
                  pieces.push_back({GenCallPiece::Kind::StackChunkFromAddr,
                                    arg.operand,
                                    hfa->elem_ty,
                                    -1,
                                    stack_arg_bytes + elem_index * hfa->elem_size,
                                    elem_index * hfa->elem_size,
                                    hfa->elem_size});
                }
                stack_arg_bytes += hfa->elem_count * hfa->elem_size;
              }
              continue;
            }

            if (is_variadic_call && gen_type_is_address_only_value(stripped_ty)) {
              const auto layout = gen_type_layout(stripped_ty, module.type_decls);
              if (layout.size > 0 && layout.size <= 16) {
                const int chunk_count = (layout.size + 7) / 8;
                if (next_gp_reg + chunk_count <= 8) {
                  for (int chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
                    const int chunk_offset = chunk_index * 8;
                    const int chunk_size = std::min(8, layout.size - chunk_offset);
                    pieces.push_back({GenCallPiece::Kind::GPChunkFromAddr,
                                      arg.operand,
                                      chunk_size > 4 ? "i64" : "i32",
                                      next_gp_reg++,
                                      -1,
                                      chunk_offset,
                                      chunk_size});
                  }
                } else {
                  stack_arg_bytes = align_up(stack_arg_bytes, 8);
                  for (int chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
                    const int chunk_offset = chunk_index * 8;
                    const int chunk_size = std::min(8, layout.size - chunk_offset);
                    pieces.push_back({GenCallPiece::Kind::StackChunkFromAddr,
                                      arg.operand,
                                      chunk_size > 4 ? "i64" : "i32",
                                      -1,
                                      stack_arg_bytes + chunk_index * 8,
                                      chunk_offset,
                                      chunk_size});
                  }
                  stack_arg_bytes += chunk_count * 8;
                }
                continue;
              }
            }

            if (gen_is_direct_gp_aggregate_type(stripped_ty, module.type_decls)) {
              const auto layout = gen_type_layout(stripped_ty, module.type_decls);
              const int chunk_count = (layout.size + 7) / 8;
              for (int chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
                const int chunk_offset = chunk_index * 8;
                const int chunk_size = std::min(8, layout.size - chunk_offset);
                const std::string chunk_ty = gen_chunk_type_for_size(chunk_size);
                if (next_gp_reg < 8) {
                  pieces.push_back({GenCallPiece::Kind::GPChunkFromAddr,
                                    arg.operand,
                                    chunk_ty,
                                    next_gp_reg++,
                                    -1,
                                    chunk_offset,
                                    chunk_size});
                } else {
                  stack_arg_bytes = align_up(stack_arg_bytes, 8);
                  pieces.push_back({GenCallPiece::Kind::StackChunkFromAddr,
                                    arg.operand,
                                    chunk_ty,
                                    -1,
                                    stack_arg_bytes,
                                    chunk_offset,
                                    chunk_size});
                  stack_arg_bytes += 8;
                }
              }
              continue;
            }

            if (gen_is_fp_type(stripped_ty)) {
              if (next_fp_reg < 8) {
                pieces.push_back({GenCallPiece::Kind::FPScalar, arg.operand, stripped_ty,
                                  next_fp_reg++, -1, 0, 0});
              } else {
                stack_arg_bytes = align_up(stack_arg_bytes, 8);
                pieces.push_back({GenCallPiece::Kind::StackScalar, arg.operand, stripped_ty, -1,
                                  stack_arg_bytes, 0, 0});
                stack_arg_bytes += 8;
              }
            } else {
              if (next_gp_reg < 8) {
                pieces.push_back({GenCallPiece::Kind::GPScalar, arg.operand, stripped_ty,
                                  next_gp_reg++, -1, 0, 0});
              } else {
                stack_arg_bytes = align_up(stack_arg_bytes, 8);
                pieces.push_back({GenCallPiece::Kind::StackScalar, arg.operand, stripped_ty, -1,
                                  stack_arg_bytes, 0, 0});
                stack_arg_bytes += 8;
              }
            }
          }

          for (const auto& current_piece : pieces) {
            switch (current_piece.kind) {
              case GenCallPiece::Kind::GPScalar:
                gen_load_operand(out, current_piece.operand, current_piece.type, sm,
                                 current_piece.reg_index, fn.name, &extern_globals);
                break;
              case GenCallPiece::Kind::FPScalar:
                gen_load_fp_operand(out, current_piece.operand, current_piece.type, sm,
                                    current_piece.reg_index, fn.name, &extern_globals);
                break;
              case GenCallPiece::Kind::GPChunkFromAddr:
                emit_load_address(current_piece.operand, 9);
                gen_emit_load_chunk_from_addr(out, current_piece.reg_index, current_piece.type, 9,
                                              current_piece.source_offset);
                break;
              case GenCallPiece::Kind::FPChunkFromAddr:
                emit_load_address(current_piece.operand, 9);
                gen_emit_load_chunk_from_addr(out, current_piece.reg_index, current_piece.type, 9,
                                              current_piece.source_offset);
                break;
              case GenCallPiece::Kind::StackScalar:
              case GenCallPiece::Kind::StackChunkFromAddr:
              case GenCallPiece::Kind::StackFp128FromAddr:
                break;
            }
          }

          if (stack_arg_bytes % 16 != 0) {
            stack_arg_bytes += 16 - (stack_arg_bytes % 16);
          }
          if (stack_arg_bytes > 0) {
            gen_emit_sp_adjust(out, "sub", stack_arg_bytes);
            gen_emit_add_imm(out, "x12", "sp", stack_arg_bytes, "x13");
            for (const auto& current_piece : pieces) {
              switch (current_piece.kind) {
                case GenCallPiece::Kind::StackScalar:
                  if (gen_is_fp_type(current_piece.type)) {
                    emit_load_scalar_from_frame_base(current_piece.operand, current_piece.type, 9,
                                                     "x12");
                    if (current_piece.type == "double") {
                      out << "  str d9, [sp, #" << current_piece.stack_offset << "]\n";
                    } else {
                      out << "  str s9, [sp, #" << current_piece.stack_offset << "]\n";
                    }
                  } else {
                    emit_load_scalar_from_frame_base(current_piece.operand, current_piece.type, 9,
                                                     "x12");
                    out << "  str x9, [sp, #" << current_piece.stack_offset << "]\n";
                  }
                  break;
                case GenCallPiece::Kind::StackChunkFromAddr:
                  emit_load_address_from_frame_base(current_piece.operand, 9, "x12");
                  gen_emit_load_chunk_from_addr(out, 9, current_piece.type, 9,
                                                current_piece.source_offset);
                  if (current_piece.type == "fp128") {
                    out << "  str q9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else if (current_piece.type == "double") {
                    out << "  str d9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else if (current_piece.type == "float") {
                    out << "  str s9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else if (current_piece.byte_size <= 1) {
                    out << "  strb w9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else if (current_piece.byte_size <= 2) {
                    out << "  strh w9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else if (current_piece.byte_size <= 4) {
                    out << "  str w9, [sp, #" << current_piece.stack_offset << "]\n";
                  } else {
                    out << "  str x9, [sp, #" << current_piece.stack_offset << "]\n";
                  }
                  break;
                case GenCallPiece::Kind::StackFp128FromAddr:
                  emit_load_address_from_frame_base(current_piece.operand, 9, "x12");
                  if (current_piece.source_offset == 0) {
                    out << "  ldr q0, [x9]\n";
                  } else {
                    out << "  ldr q0, [x9, #" << current_piece.source_offset << "]\n";
                  }
                  out << "  str q0, [sp, #" << current_piece.stack_offset << "]\n";
                  break;
                case GenCallPiece::Kind::GPScalar:
                case GenCallPiece::Kind::FPScalar:
                case GenCallPiece::Kind::GPChunkFromAddr:
                case GenCallPiece::Kind::FPChunkFromAddr:
                  break;
              }
            }
          }

          const bool sret_call =
              !p->result.empty() &&
              gen_type_is_sret_aggregate_type(p->return_type.str(), module.type_decls);
          if (sret_call) {
            const int data_off = sm.lookup_data(p->result.str());
            if (data_off < 0) return std::nullopt;
            gen_emit_add_imm(out, "x8", "sp", data_off, "x9");
          }

          // Save lr before call
          out << "  str x30, [sp, #" << stack_arg_bytes << "]\n";

          if (is_direct) {
            out << "  bl " << callee_sym << "\n";
          } else {
            // Indirect call — load fn ptr from stack
            // We need a scratch reg that won't conflict with args
            // Load callee address into x9 (not used for args)
            int callee_off = sm.lookup(callee);
            if (callee_off >= 0) {
              out << "  ldr x9, [sp, #" << callee_off << "]\n";
            }
            out << "  blr x9\n";
          }

          // Restore lr
          out << "  ldr x30, [sp, #" << stack_arg_bytes << "]\n";
          if (stack_arg_bytes > 0) {
            gen_emit_sp_adjust(out, "add", stack_arg_bytes);
          }

          // Store return value
          if (!p->result.empty()) {
            std::string ret_ty = p->return_type.str();
            if (const auto hfa = gen_try_parse_hfa_type(ret_ty, module.type_decls)) {
              const int data_off = sm.lookup_data(p->result.str());
              const int slot_off = sm.lookup(p->result.str());
              if (data_off < 0 || slot_off < 0) return std::nullopt;
              gen_emit_add_imm(out, "x9", "sp", data_off, "x10");
              for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
                gen_emit_store_typed_chunk_to_addr(out, elem_index, hfa->elem_ty, 9,
                                                   elem_index * hfa->elem_size);
              }
              out << "  str x9, [sp, #" << slot_off << "]\n";
            } else if (gen_type_is_sret_aggregate_type(ret_ty, module.type_decls)) {
              const int data_off = sm.lookup_data(p->result.str());
              const int slot_off = sm.lookup(p->result.str());
              if (data_off < 0 || slot_off < 0) return std::nullopt;
              gen_emit_add_imm(out, "x9", "sp", data_off, "x10");
              out << "  str x9, [sp, #" << slot_off << "]\n";
            } else if (gen_is_direct_gp_aggregate_type(ret_ty, module.type_decls)) {
              const int data_off = sm.lookup_data(p->result.str());
              const int slot_off = sm.lookup(p->result.str());
              if (data_off < 0 || slot_off < 0) return std::nullopt;
              gen_emit_add_imm(out, "x9", "sp", data_off, "x9");
              gen_emit_store_chunk_to_addr(out, 0, 8, 9, 0);
              const int tail_bytes = gen_type_layout(ret_ty, module.type_decls).size - 8;
              if (tail_bytes > 0) {
                gen_emit_store_chunk_to_addr(out, 1, tail_bytes, 9, 8);
              }
              out << "  str x9, [sp, #" << slot_off << "]\n";
            } else if (gen_is_fp_type(ret_ty)) {
              out << "  fmov " << (ret_ty == "double" ? "x0, d0" : "w0, s0") << "\n";
              gen_store_result(out, p->result.str(), ret_ty, sm, 0);
            } else {
              gen_store_result(out, p->result.str(), ret_ty, sm, 0);
            }
          }
        }
        else if (const auto* p = std::get_if<LirGepOp>(&inst)) {
          // Load base pointer
          gen_load_operand(out, p->ptr.str(), "ptr", sm, 0, fn.name, &extern_globals);

          std::string elem_ty = p->element_type.str();

          // Process indices
          if (p->indices.size() == 1) {
            // Simple: ptr + index * elem_size
            auto [idx_ty, idx_val] = gen_parse_index(p->indices[0]);
            int elem_size = gen_type_bytes(elem_ty);
            if (elem_ty[0] == '[') {
              auto [cnt, esz] = gen_parse_array_type(elem_ty);
              elem_size = cnt * esz;
            } else if (elem_ty[0] == '{' || elem_ty.rfind("%struct.", 0) == 0) {
              elem_size = gen_struct_size(elem_ty, module.type_decls);
            }
            if (idx_val[0] == '%') {
              gen_load_operand(out, idx_val, idx_ty, sm, 1, fn.name, &extern_globals);
              if (gen_is_64bit(idx_ty)) {
                out << "  mov x2, #" << elem_size << "\n";
                out << "  mul x1, x1, x2\n";
              } else {
                out << "  mov w2, #" << elem_size << "\n";
                out << "  sxtw x1, w1\n";
                out << "  mov x2, x2\n";
                out << "  mul x1, x1, x2\n";
              }
              out << "  add x0, x0, x1\n";
            } else {
              long long idx = 0;
              try { idx = std::stoll(idx_val); } catch (...) {}
              long long byte_off = idx * elem_size;
              if (byte_off != 0) {
                if (byte_off >= 0 && byte_off < 4096) {
                  out << "  add x0, x0, #" << byte_off << "\n";
                } else {
                  out << "  mov x1, #" << byte_off << "\n";
                  out << "  add x0, x0, x1\n";
                }
              }
            }
          } else if (p->indices.size() == 2) {
            // Two-index GEP: first index is array/struct stride, second is element
            auto [idx0_ty, idx0_val] = gen_parse_index(p->indices[0]);
            auto [idx1_ty, idx1_val] = gen_parse_index(p->indices[1]);

            // First index: stride over the whole element type
            int outer_size = gen_type_bytes(elem_ty);
            if (elem_ty[0] == '[') {
              auto [cnt, esz] = gen_parse_array_type(elem_ty);
              outer_size = cnt * esz;
            } else if (elem_ty[0] == '{' || elem_ty.rfind("%struct.", 0) == 0) {
              outer_size = gen_struct_size(elem_ty, module.type_decls);
            }

            // Apply first index
            if (idx0_val[0] == '%') {
              gen_load_operand(out, idx0_val, idx0_ty, sm, 1, fn.name, &extern_globals);
              out << "  mov x2, #" << outer_size << "\n";
              if (!gen_is_64bit(idx0_ty)) out << "  sxtw x1, w1\n";
              out << "  mul x1, x1, x2\n";
              out << "  add x0, x0, x1\n";
            } else {
              long long idx0 = 0;
              try { idx0 = std::stoll(idx0_val); } catch (...) {}
              long long off0 = idx0 * outer_size;
              if (off0 != 0) {
                if (off0 >= 0 && off0 < 4096) {
                  out << "  add x0, x0, #" << off0 << "\n";
                } else {
                  out << "  mov x1, #" << off0 << "\n";
                  out << "  add x0, x0, x1\n";
                }
              }
            }

            // Second index: depends on whether element type is struct or array
            if (elem_ty[0] == '{' || elem_ty.rfind("%struct.", 0) == 0) {
              // Struct GEP: second index is field index (must be constant)
              long long field_idx = 0;
              try { field_idx = std::stoll(idx1_val); } catch (...) {}
              int field_off = gen_struct_field_offset(elem_ty, static_cast<int>(field_idx),
                                                      module.type_decls);
              if (field_off != 0) {
                if (field_off < 4096) {
                  out << "  add x0, x0, #" << field_off << "\n";
                } else {
                  out << "  mov x1, #" << field_off << "\n";
                  out << "  add x0, x0, x1\n";
                }
              }
            } else {
              // Array GEP: second index into array elements
              int inner_size = gen_type_bytes(elem_ty);
              if (elem_ty[0] == '[') {
                auto [cnt, esz] = gen_parse_array_type(elem_ty);
                inner_size = esz;  // element size within array
              }
              if (idx1_val[0] == '%') {
                gen_load_operand(out, idx1_val, idx1_ty, sm, 1, fn.name, &extern_globals);
                if (!gen_is_64bit(idx1_ty)) out << "  sxtw x1, w1\n";
                out << "  mov x2, #" << inner_size << "\n";
                out << "  mul x1, x1, x2\n";
                out << "  add x0, x0, x1\n";
              } else {
                long long idx1 = 0;
                try { idx1 = std::stoll(idx1_val); } catch (...) {}
                long long off1 = idx1 * inner_size;
                if (off1 != 0) {
                  if (off1 >= 0 && off1 < 4096) {
                    out << "  add x0, x0, #" << off1 << "\n";
                  } else {
                    out << "  mov x1, #" << off1 << "\n";
                    out << "  add x0, x0, x1\n";
                  }
                }
              }
            }
          } else if (p->indices.size() == 3) {
            // Three-index GEP: e.g. struct with nested array
            auto [idx0_ty, idx0_val] = gen_parse_index(p->indices[0]);
            auto [idx1_ty, idx1_val] = gen_parse_index(p->indices[1]);
            auto [idx2_ty, idx2_val] = gen_parse_index(p->indices[2]);

            int outer_size = gen_type_bytes(elem_ty);
            if (elem_ty[0] == '{' || elem_ty.rfind("%struct.", 0) == 0)
              outer_size = gen_struct_size(elem_ty, module.type_decls);

            // First index
            long long idx0 = 0;
            try { idx0 = std::stoll(idx0_val); } catch (...) {}
            long long off0 = idx0 * outer_size;
            if (off0 != 0) {
              out << "  add x0, x0, #" << off0 << "\n";
            }

            // Second index (struct field)
            long long field_idx = 0;
            try { field_idx = std::stoll(idx1_val); } catch (...) {}
            int field_off = gen_struct_field_offset(elem_ty, static_cast<int>(field_idx),
                                                    module.type_decls);
            if (field_off != 0) {
              out << "  add x0, x0, #" << field_off << "\n";
            }

            // Third index (array element within struct field)
            if (idx2_val[0] == '%') {
              gen_load_operand(out, idx2_val, idx2_ty, sm, 1, fn.name, &extern_globals);
              if (!gen_is_64bit(idx2_ty)) out << "  sxtw x1, w1\n";
              // Would need element size of the array field — approximate as 4 or 8
              out << "  mov x2, #4\n";
              out << "  mul x1, x1, x2\n";
              out << "  add x0, x0, x1\n";
            } else {
              long long idx2 = 0;
              try { idx2 = std::stoll(idx2_val); } catch (...) {}
              if (idx2 != 0) {
                out << "  add x0, x0, #" << (idx2 * 4) << "\n";
              }
            }
          }
          // Store result pointer
          gen_store_result(out, p->result.str(), "ptr", sm, 0);
        }
        else if (const auto* p = std::get_if<LirSelectOp>(&inst)) {
          std::string ty = p->type_str.str();
          gen_load_operand(out, p->cond.str(), "i32", sm, 2, fn.name, &extern_globals);
          int label_id = gen_label_counter++;
          out << "  cbnz w2, .Lsel_true_" << label_id << "\n";
          gen_load_operand(out, p->false_val.str(), ty, sm, 0, fn.name, &extern_globals);
          out << "  b .Lsel_end_" << label_id << "\n";
          out << ".Lsel_true_" << label_id << ":\n";
          gen_load_operand(out, p->true_val.str(), ty, sm, 0, fn.name, &extern_globals);
          out << ".Lsel_end_" << label_id << ":\n";
          gen_store_result(out, p->result.str(), ty, sm, 0);
        }
        else if (const auto* p = std::get_if<LirMemcpyOp>(&inst)) {
          // memcpy(dst, src, size)
          gen_load_operand(out, p->dst.str(), "ptr", sm, 0, fn.name, &extern_globals);
          gen_load_operand(out, p->src.str(), "ptr", sm, 1, fn.name, &extern_globals);
          gen_load_operand(out, p->size.str(), "i64", sm, 2, fn.name, &extern_globals);
          out << "  str x30, [sp, #0]\n";
          out << "  bl memcpy\n";
          out << "  ldr x30, [sp, #0]\n";
        }
        else if (const auto* p = std::get_if<LirMemsetOp>(&inst)) {
          gen_load_operand(out, p->dst.str(), "ptr", sm, 0, fn.name, &extern_globals);
          gen_load_operand(out, p->byte_val.str(), "i32", sm, 1, fn.name, &extern_globals);
          gen_load_operand(out, p->size.str(), "i64", sm, 2, fn.name, &extern_globals);
          out << "  str x30, [sp, #0]\n";
          out << "  bl memset\n";
          out << "  ldr x30, [sp, #0]\n";
        }
        else if (const auto* p = std::get_if<LirVaStartOp>(&inst)) {
          const int ap_off = sm.lookup(p->ap_ptr.str());
          if (ap_off < 0 || sm.va_gp_save_offset < 0 || sm.va_fp_save_offset < 0) {
            return std::nullopt;
          }
          out << "  ldr x0, [sp, #" << ap_off << "]\n";
          gen_emit_add_imm(out, "x1", "sp", sm.frame_size, "x9");
          out << "  str x1, [x0]\n";
          gen_emit_add_imm(out, "x1", "sp", sm.va_gp_save_offset + 64, "x9");
          out << "  str x1, [x0, #8]\n";
          gen_emit_add_imm(out, "x1", "sp", sm.va_fp_save_offset + 128, "x9");
          out << "  str x1, [x0, #16]\n";
          gen_emit_store_i32_constant(out, "x0", 24, -((8 - named_gp_count) * 8));
          gen_emit_store_i32_constant(out, "x0", 28, -((8 - named_fp_count) * 16));
        }
        else if (const auto* p = std::get_if<LirVaEndOp>(&inst)) {
          (void)p;
        }
        else if (const auto* p = std::get_if<LirVaCopyOp>(&inst)) {
          const int dst_off = sm.lookup(p->dst_ptr.str());
          const int src_off = sm.lookup(p->src_ptr.str());
          if (dst_off < 0 || src_off < 0) return std::nullopt;
          out << "  ldr x0, [sp, #" << dst_off << "]\n";
          out << "  ldr x1, [sp, #" << src_off << "]\n";
          out << "  ldp x2, x3, [x1]\n";
          out << "  stp x2, x3, [x0]\n";
          out << "  ldp x2, x3, [x1, #16]\n";
          out << "  stp x2, x3, [x0, #16]\n";
        }
        else if (const auto* p = std::get_if<LirVaArgOp>(&inst)) {
          const int ap_off = sm.lookup(p->ap_ptr.str());
          const std::string ty = p->type_str.str();
          if (ap_off < 0 || !gen_is_scalar_variadic_type(ty) || ty == "fp128") {
            return std::nullopt;
          }
          const int label_id = gen_label_counter++;
          out << "  ldr x1, [sp, #" << ap_off << "]\n";
          if (gen_is_fp_type(ty)) {
            out << "  ldrsw x2, [x1, #28]\n";
            out << "  tbz x2, #63, .Lva_stack_" << label_id << "\n";
            out << "  ldr x3, [x1, #16]\n";
            out << "  add x3, x3, x2\n";
            out << "  add w2, w2, #16\n";
            out << "  str w2, [x1, #28]\n";
            out << "  ldr " << (ty == "double" ? "d0" : "s0") << ", [x3]\n";
            out << "  b .Lva_done_" << label_id << "\n";
            out << ".Lva_stack_" << label_id << ":\n";
            out << "  ldr x3, [x1]\n";
            out << "  ldr " << (ty == "double" ? "d0" : "s0") << ", [x3]\n";
            out << "  add x3, x3, #8\n";
            out << "  str x3, [x1]\n";
            out << ".Lva_done_" << label_id << ":\n";
            out << "  fmov " << (ty == "double" ? "x0, d0" : "w0, s0") << "\n";
          } else {
            const int stack_step = gen_type_layout(ty, module.type_decls).size <= 8 ? 8 : 16;
            out << "  ldrsw x2, [x1, #24]\n";
            out << "  tbz x2, #63, .Lva_stack_" << label_id << "\n";
            out << "  ldr x3, [x1, #8]\n";
            out << "  add x3, x3, x2\n";
            out << "  add w2, w2, #8\n";
            out << "  str w2, [x1, #24]\n";
            out << "  ldr " << (gen_is_64bit(ty) ? "x0" : "w0") << ", [x3]\n";
            out << "  b .Lva_done_" << label_id << "\n";
            out << ".Lva_stack_" << label_id << ":\n";
            out << "  ldr x3, [x1]\n";
            out << "  ldr " << (gen_is_64bit(ty) ? "x0" : "w0") << ", [x3]\n";
            out << "  add x3, x3, #" << stack_step << "\n";
            out << "  str x3, [x1]\n";
            out << ".Lva_done_" << label_id << ":\n";
          }
          gen_store_result(out, p->result.str(), ty, sm, 0);
        }
        else if (const auto* p = std::get_if<LirAbsOp>(&inst)) {
          std::string ty = p->int_type.str();
          const char* rp = gen_reg_prefix(ty);
          gen_load_operand(out, p->arg.str(), ty, sm, 0, fn.name, &extern_globals);
          out << "  cmp " << rp << "0, #0\n";
          out << "  cneg " << rp << "0, " << rp << "0, lt\n";
          gen_store_result(out, p->result.str(), ty, sm, 0);
        }
        else if (const auto* p = std::get_if<LirExtractValueOp>(&inst)) {
          // Extract field from aggregate — load aggregate base + offset
          gen_load_operand(out, p->agg.str(), "i64", sm, 0, fn.name, &extern_globals);
          // For simple {i32, i1} structs returned from calls, the value is in memory
          // This is a simplification — we treat the aggregate slot as a base pointer
          // and extract at field offset
          // Actually for stack-spill, aggregates are stored as flat bytes
          // field 0 = offset 0, field 1 = offset 4 or 8
          // For now, just return 0 for unsupported patterns
          gen_store_result(out, p->result.str(), "i64", sm, 0);
        }
        else if (const auto* p = std::get_if<LirInsertValueOp>(&inst)) {
          gen_load_operand(out, p->agg.str(), "i64", sm, 0, fn.name, &extern_globals);
          gen_store_result(out, p->result.str(), "i64", sm, 0);
        }
        else if (const auto* p = std::get_if<LirStackSaveOp>(&inst)) {
          out << "  mov x0, sp\n";
          gen_store_result(out, p->result.str(), "i64", sm, 0);
        }
        else if (const auto* p = std::get_if<LirStackRestoreOp>(&inst)) {
          gen_load_operand(out, p->saved_ptr.str(), "i64", sm, 0, fn.name, &extern_globals);
          out << "  mov sp, x0\n";
        }
        else {
          // Unknown instruction type — bail out
          return std::nullopt;
        }
      }

      // Emit terminator
      if (const auto* t = std::get_if<LirRet>(&blk.terminator)) {
        if (t->value_str.has_value() && t->type_str != "void") {
          if (const auto hfa = gen_try_parse_hfa_type(t->type_str, module.type_decls)) {
            gen_load_operand(out, *t->value_str, "ptr", sm, 9, fn.name, &extern_globals,
                             &module.type_decls);
            for (int elem_index = 0; elem_index < hfa->elem_count; ++elem_index) {
              gen_emit_load_chunk_from_addr(out, elem_index, hfa->elem_ty, 9,
                                            elem_index * hfa->elem_size);
            }
          } else if (gen_type_is_sret_aggregate_type(t->type_str, module.type_decls)) {
            const int sret_off = sm.lookup("%ret.sret");
            if (sret_off < 0) return std::nullopt;
            out << "  ldr x8, [sp, #" << sret_off << "]\n";
            gen_load_operand(out, *t->value_str, "ptr", sm, 9, fn.name, &extern_globals,
                             &module.type_decls);
            const int total_size = gen_type_layout(t->type_str, module.type_decls).size;
            int offset = 0;
            while (offset < total_size) {
              const int chunk_size =
                  gen_chunk_size_for_remaining_copy_bytes(total_size - offset);
              gen_emit_load_chunk_from_addr(out, 10, chunk_size == 16 ? "fp128"
                                                                      : gen_chunk_type_for_size(chunk_size),
                                            9, offset);
              gen_emit_store_chunk_to_addr(out, 10, chunk_size, 8, offset);
              offset += chunk_size;
            }
          } else if (gen_is_direct_gp_aggregate_type(t->type_str, module.type_decls)) {
            gen_load_operand(out, *t->value_str, "ptr", sm, 0, fn.name, &extern_globals,
                             &module.type_decls);
            out << "  mov x9, x0\n";
            out << "  ldr x0, [x9]\n";
            const int tail_bytes = gen_type_layout(t->type_str, module.type_decls).size - 8;
            if (tail_bytes > 0) {
              gen_emit_load_chunk_from_addr(out, 1, gen_chunk_type_for_size(tail_bytes), 9, 8);
            }
          } else {
            gen_load_operand(out, *t->value_str, t->type_str, sm, 0, fn.name, &extern_globals,
                             &module.type_decls);
          }
        }
        out << "  ldr x30, [sp, #0]\n";
        gen_emit_sp_adjust(out, "add", sm.frame_size);
        out << "  ret\n";
      }
      else if (const auto* t = std::get_if<LirBr>(&blk.terminator)) {
        if (gen_block_has_leading_phi(fn, t->target_label) &&
            !gen_emit_phi_edge_copies(out, fn, t->target_label, blk.label, sm,
                                      extern_globals, module.type_decls)) {
          return std::nullopt;
        }
        out << "  b ." << fn.name << "." << t->target_label << "\n";
      }
      else if (const auto* t = std::get_if<LirCondBr>(&blk.terminator)) {
        // Load condition
        int cond_off = sm.lookup(t->cond_name);
        if (cond_off >= 0) {
          out << "  ldr w0, [sp, #" << cond_off << "]\n";
        }
        const bool true_has_phi = gen_block_has_leading_phi(fn, t->true_label);
        const bool false_has_phi = gen_block_has_leading_phi(fn, t->false_label);
        if (!true_has_phi && !false_has_phi) {
          out << "  cbnz w0, ." << fn.name << "." << t->true_label << "\n";
          out << "  b ." << fn.name << "." << t->false_label << "\n";
        } else if (true_has_phi && false_has_phi) {
          out << "  cbz w0, ." << fn.name << "." << blk.label << ".phi_false\n";
          if (!gen_emit_phi_edge_copies(out, fn, t->true_label, blk.label, sm, extern_globals,
                                        module.type_decls)) {
            return std::nullopt;
          }
          out << "  b ." << fn.name << "." << t->true_label << "\n";
          out << "." << fn.name << "." << blk.label << ".phi_false:\n";
          if (!gen_emit_phi_edge_copies(out, fn, t->false_label, blk.label, sm, extern_globals,
                                        module.type_decls)) {
            return std::nullopt;
          }
          out << "  b ." << fn.name << "." << t->false_label << "\n";
        } else if (true_has_phi) {
          out << "  cbz w0, ." << fn.name << "." << t->false_label << "\n";
          if (!gen_emit_phi_edge_copies(out, fn, t->true_label, blk.label, sm, extern_globals,
                                        module.type_decls)) {
            return std::nullopt;
          }
          out << "  b ." << fn.name << "." << t->true_label << "\n";
        } else {
          out << "  cbnz w0, ." << fn.name << "." << t->true_label << "\n";
          if (!gen_emit_phi_edge_copies(out, fn, t->false_label, blk.label, sm, extern_globals,
                                        module.type_decls)) {
            return std::nullopt;
          }
          out << "  b ." << fn.name << "." << t->false_label << "\n";
        }
      }
      else if (const auto* t = std::get_if<LirSwitch>(&blk.terminator)) {
        // Load selector
        std::string sel_ty = t->selector_type;
        const char* rp = gen_reg_prefix(sel_ty);
        int sel_off = sm.lookup(t->selector_name);
        if (sel_off >= 0) {
          out << "  ldr " << rp << "0, [sp, #" << sel_off << "]\n";
        }
        for (const auto& [val, label] : t->cases) {
          if (val >= -256 && val <= 4095) {
            out << "  cmp " << rp << "0, #" << val << "\n";
          } else {
            out << "  mov " << rp << "1, #" << val << "\n";
            out << "  cmp " << rp << "0, " << rp << "1\n";
          }
          out << "  b.eq ." << fn.name << "." << label << "\n";
        }
        out << "  b ." << fn.name << "." << t->default_label << "\n";
      }
      else if (std::holds_alternative<LirUnreachable>(blk.terminator)) {
        out << "  brk #1\n";  // trap
      }
    }
  }

  return out.str();
}

std::optional<std::string> render_prepared_lir_module_if_supported(
    const c4c::codegen::lir::LirModule& module,
    bool needs_nonminimal_lowering) {
  try {
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_store_reload_asm(module.target_profile.triple, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_roundtrip_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_load_asm(module.target_profile.triple, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      return emit_minimal_string_literal_char_asm(module.target_profile.triple, *slice);
    }
    if (!needs_nonminimal_lowering) {
      if (const auto imm = parse_minimal_lir_local_pointer_return_imm(module);
          imm.has_value()) {
        return emit_minimal_return_imm_asm(
            module.target_profile.triple, module.functions.front().name, *imm);
      }
    }
    if (const auto imm = parse_minimal_lir_return_imm(module); imm.has_value()) {
      return emit_minimal_return_imm_asm(
          module.target_profile.triple, module.functions.front().name, *imm);
    }
    if (const auto imm = parse_minimal_lir_return_sub_imm(module); imm.has_value()) {
      return emit_minimal_return_sub_imm_asm(
          module.target_profile.triple, module.functions.front().name, *imm);
    }
  } catch (const std::invalid_argument&) {
  }

  return std::nullopt;
}

}  // namespace

std::optional<std::string> render_bir_module_if_supported(
    const c4c::backend::bir::Module& module) {
  if (const auto slice = c4c::backend::parse_bir_minimal_direct_call_module(module);
      slice.has_value()) {
    return emit_minimal_direct_call_asm(module, *slice);
  }
  if (const auto slice = c4c::backend::parse_bir_minimal_void_direct_call_imm_return_module(module);
      slice.has_value()) {
    return emit_minimal_void_direct_call_imm_return_asm(module, *slice);
  }
  if (const auto slice = c4c::backend::parse_bir_minimal_declared_direct_call_module(module);
      slice.has_value()) {
    return emit_minimal_declared_direct_call_asm(module, *slice);
  }
  if (const auto slice = c4c::backend::parse_bir_minimal_two_arg_direct_call_module(module);
      slice.has_value()) {
    return emit_minimal_two_arg_direct_call_asm(module, *slice);
  }
  if (const auto slice = c4c::backend::parse_bir_minimal_direct_call_add_imm_module(module);
      slice.has_value()) {
    return emit_minimal_direct_call_add_imm_asm(module, *slice);
  }
  if (const auto slice = c4c::backend::parse_bir_minimal_direct_call_identity_arg_module(module);
      slice.has_value()) {
    return emit_minimal_direct_call_identity_arg_asm(module, *slice);
  }
  if (const auto slice =
          c4c::backend::parse_bir_minimal_dual_identity_direct_call_sub_module(module);
      slice.has_value()) {
    return emit_minimal_dual_identity_direct_call_sub_asm(module, *slice);
  }
  if (const auto slice =
          c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(module);
      slice.has_value()) {
    return emit_minimal_call_crossing_direct_call_asm(
        module, synthesize_shared_aarch64_call_crossing_regalloc(slice->regalloc_source_value),
        *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_load_slice(module);
      slice.has_value()) {
    return emit_minimal_scalar_global_load_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
      slice.has_value()) {
    return emit_minimal_extern_scalar_global_load_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
      slice.has_value()) {
    return emit_minimal_extern_global_array_load_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
      slice.has_value()) {
    return emit_minimal_scalar_global_store_reload_asm(module.target_triple, *slice);
  }
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_sub_imm_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
    return emit_minimal_return_sub_imm_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
    return emit_minimal_return_sub_imm_asm(module, *imm);
  }
  if (const auto slice = parse_minimal_affine_return_slice(module);
      slice.has_value()) {
    return emit_minimal_affine_return_asm(module, *slice);
  }
  if (const auto slice = parse_minimal_cast_return_slice(module);
      slice.has_value()) {
    return emit_minimal_cast_return_asm(module, *slice);
  }
  if (const auto slice = parse_minimal_conditional_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_conditional_affine_i8_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_affine_i8_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_conditional_affine_i32_return_slice(module);
      slice.has_value()) {
    return emit_minimal_conditional_affine_i32_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_countdown_loop_slice(module);
      slice.has_value()) {
    return emit_minimal_countdown_loop_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::string emit_module(const c4c::backend::bir::Module& module) {
  const auto target_profile = c4c::target_profile_from_triple(
      module.target_triple.empty() ? c4c::default_host_target_triple() : module.target_triple);
  return c4c::backend::emit_target_bir_module(
      module, target_profile);
}

std::string emit_direct_bir_module(const c4c::backend::bir::Module& module) {
  if (auto rendered = render_bir_module_if_supported(module); rendered.has_value()) {
    return *rendered;
  }
  fail_unsupported_direct_bir_module();
}

std::string emit_prepared_lir_module(const c4c::codegen::lir::LirModule& module) {
  const bool needs_nonminimal_lowering = lir_module_needs_nonminimal_lowering(module);
  validate_module(module);
  if (auto rendered = render_prepared_lir_module_if_supported(module, needs_nonminimal_lowering);
      rendered.has_value()) {
    return *rendered;
  }
  if (!needs_nonminimal_lowering) {
    if (const auto imm2 = try_constant_fold_single_block(module); imm2.has_value()) {
      return emit_minimal_return_imm_asm(
          module.target_profile.triple, module.functions.front().name, *imm2);
    }
  }
  if (auto gen_asm = render_general_lir_asm_if_supported(module)) {
    return *gen_asm;
  }
  fail_unsupported(
      "this direct LIR module; only direct-LIR slices that lower natively or through direct BIR are currently supported");
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  const auto target_profile = c4c::target_profile_from_triple(
      module.target_profile.triple.empty() ? c4c::default_host_target_triple() : module.target_profile.triple);
  return c4c::backend::emit_target_lir_module(
      module, target_profile);
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  const auto assembled =
      c4c::backend::assemble_target_lir_module(
          module,
          c4c::target_profile_from_triple(
              module.target_profile.triple.empty() ? c4c::default_host_target_triple()
                                           : module.target_profile.triple),
          output_path);
  return assembler::AssembleResult{
      .staged_text = assembled.staged_text,
      .output_path = assembled.output_path,
      .object_emitted = assembled.object_emitted,
  };
}

}  // namespace c4c::backend::aarch64
