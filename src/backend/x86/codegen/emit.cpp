#include "emit.hpp"

#include "../../bir.hpp"
#include "../../generation.hpp"
#include "../../ir_printer.hpp"
#include "../../ir_validate.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../lowering/lir_to_backend_ir.hpp"
#include "../../lowering/lir_to_bir.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string_view>

// Mechanical translation of the ref x86 emitter entrypoint.
// The larger target-specific lowering surface is split across the sibling
// translation units in this directory.

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

bool is_direct_bir_subset_error(const std::invalid_argument& ex) {
  return std::string_view(ex.what()).find("does not support this direct BIR module") !=
         std::string_view::npos;
}

std::string asm_symbol_name(const c4c::backend::BackendModule& module,
                            std::string_view logical_name);
std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name);
std::string asm_private_data_label(const c4c::backend::BackendModule& module,
                                   std::string_view pool_name);
std::string asm_private_data_label(std::string_view target_triple,
                                   std::string_view pool_name);

const std::string& synthetic_call_crossing_regalloc_source_value() {
  static const std::string kSyntheticCallCrossingRegallocSource =
      "%t.call_crossing.regalloc_source";
  return kSyntheticCallCrossingRegallocSource;
}

struct MinimalCallCrossingDirectCallSlice {
  std::string callee_name;
  std::string caller_name = "main";
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
  std::string regalloc_source_value = synthetic_call_crossing_regalloc_source_value();
};

using MinimalExternCallArgSlice = c4c::backend::ParsedBackendExternCallArg;

struct MinimalTwoArgDirectCallSlice {
  std::string callee_name;
  std::string caller_name = "main";
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalDualIdentityDirectCallSubSlice {
  std::string lhs_helper_name;
  std::string rhs_helper_name;
  std::string caller_name = "main";
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalNamedReturnImmSlice {
  std::string function_name = "main";
  std::int64_t return_imm = 0;
};

struct MinimalMemberArrayRuntimeSlice {
  enum class Kind : unsigned char {
    ByValueParam,
    NestedMemberPointer,
  };

  Kind kind = Kind::ByValueParam;
  std::string helper_name;
  std::string caller_name = "main";
  std::int64_t first_imm = 0;
  std::int64_t second_imm = 0;
};

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

struct MinimalConditionalPhiJoinSlice {
  struct ArithmeticStep {
    c4c::backend::BackendBinaryOpcode opcode = c4c::backend::BackendBinaryOpcode::Add;
    std::int64_t imm = 0;
  };

  struct IncomingValue {
    std::int64_t base_imm = 0;
    std::vector<ArithmeticStep> steps;
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

struct MinimalCastReturnSlice {
  std::string function_name;
  c4c::backend::bir::CastOpcode opcode = c4c::backend::bir::CastOpcode::SExt;
  c4c::backend::bir::TypeKind operand_type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::TypeKind result_type = c4c::backend::bir::TypeKind::Void;
};

struct MinimalCountdownLoopSlice {
  std::int64_t initial_imm = 0;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
};

struct MinimalLocalArraySlice;
struct MinimalExternScalarGlobalLoadSlice;
struct MinimalExternGlobalArrayLoadSlice;
struct MinimalGlobalCharPointerDiffSlice;
struct MinimalGlobalIntPointerDiffSlice;
struct MinimalScalarGlobalLoadSlice;
struct MinimalScalarGlobalStoreReloadSlice;
struct MinimalMultiPrintfVarargSlice;

std::string emit_minimal_two_arg_direct_call_asm(
    std::string_view target_triple,
    const MinimalTwoArgDirectCallSlice& slice);
std::string emit_minimal_dual_identity_direct_call_sub_asm(
    std::string_view target_triple,
    const MinimalDualIdentityDirectCallSubSlice& slice);
std::string emit_minimal_member_array_runtime_asm(
    std::string_view target_triple,
    const MinimalMemberArrayRuntimeSlice& slice);
std::string emit_minimal_local_array_asm(std::string_view target_triple,
                                         const MinimalLocalArraySlice& slice);
std::string emit_minimal_extern_global_array_load_asm(
    std::string_view target_triple,
    const MinimalExternGlobalArrayLoadSlice& slice);
std::string emit_minimal_extern_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalExternScalarGlobalLoadSlice& slice);
std::string emit_minimal_global_char_pointer_diff_asm(
    std::string_view target_triple,
    const MinimalGlobalCharPointerDiffSlice& slice);
std::string emit_minimal_global_int_pointer_diff_asm(
    std::string_view target_triple,
    const MinimalGlobalIntPointerDiffSlice& slice);
std::string emit_minimal_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalLoadSlice& slice);
std::string emit_minimal_scalar_global_store_reload_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalStoreReloadSlice& slice);
std::string emit_minimal_multi_printf_vararg_asm(
    std::string_view target_triple,
    const c4c::codegen::lir::LirModule& module,
    const MinimalMultiPrintfVarargSlice& slice);

struct MinimalLocalArraySlice {
  std::int64_t store0_imm = 0;
  std::int64_t store1_imm = 0;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string global_name;
};

struct MinimalExternGlobalArrayLoadSlice {
  std::string global_name;
  std::int64_t byte_offset = 0;
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
  bool zero_initializer = false;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
};

struct MinimalMultiPrintfVarargSlice {
  std::string first_string_pool_name;
  std::string second_string_pool_name;
};

struct MinimalStringLiteralCharSlice {
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_offset = 0;
  bool sign_extend = true;
};

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
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

std::optional<std::int64_t> parse_minimal_lir_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      function.name != "main" ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
  std::unordered_map<std::string, std::string> pointer_values;
  std::unordered_map<std::string, std::string> pointer_slot_values;
  std::unordered_set<std::string> local_pointer_slots;
  for (const auto& alloca : function.alloca_insts) {
    const auto* alloca_inst = std::get_if<LirAllocaOp>(&alloca);
    if (alloca_inst == nullptr || !alloca_inst->count.empty() || alloca_inst->result.empty()) {
      continue;
    }
    if (alloca_inst->type_str == "i32") {
      if (!local_scalar_slots.insert(alloca_inst->result).second) {
        return std::nullopt;
      }
      continue;
    }
    if (alloca_inst->type_str == "ptr") {
      if (!local_pointer_slots.insert(alloca_inst->result).second) {
        return std::nullopt;
      }
    }
  }

  auto resolve_value = [&](std::string_view value) -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(value); imm.has_value()) {
      return imm;
    }
    const auto it = values.find(std::string(value));
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  auto resolve_pointer_target = [&](std::string_view operand) -> std::optional<std::string> {
    if (local_scalar_slots.find(std::string(operand)) != local_scalar_slots.end()) {
      return std::string(operand);
    }

    std::string current(operand);
    std::unordered_set<std::string> visited;
    while (true) {
      if (!visited.insert(current).second) {
        return std::nullopt;
      }
      if (local_scalar_slots.find(current) != local_scalar_slots.end()) {
        return current;
      }
      const auto value_it = pointer_values.find(current);
      if (value_it != pointer_values.end()) {
        current = value_it->second;
        continue;
      }
      const auto slot_it = pointer_slot_values.find(current);
      if (slot_it != pointer_slot_values.end()) {
        current = slot_it->second;
        continue;
      }
      return std::nullopt;
    }
  };

  auto resolve_pointer_source = [&](std::string_view operand) -> std::optional<std::string> {
    if (local_scalar_slots.find(std::string(operand)) != local_scalar_slots.end() ||
        local_pointer_slots.find(std::string(operand)) != local_pointer_slots.end()) {
      return std::string(operand);
    }
    const auto it = pointer_values.find(std::string(operand));
    if (it == pointer_values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (const auto& inst : entry.insts) {
    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str == "i32") {
        const auto target = resolve_pointer_target(store->ptr);
        if (!target.has_value()) {
          return std::nullopt;
        }
        const auto value = resolve_value(store->val);
        if (!value.has_value()) {
          return std::nullopt;
        }
        slot_values[*target] = *value;
        continue;
      }
      if (store->type_str != "ptr" ||
          local_pointer_slots.find(store->ptr) == local_pointer_slots.end()) {
        return std::nullopt;
      }
      const auto value = resolve_pointer_source(store->val);
      if (!value.has_value()) {
        return std::nullopt;
      }
      pointer_slot_values[store->ptr] = *value;
      continue;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str == "i32") {
        const auto target = resolve_pointer_target(load->ptr);
        if (!target.has_value()) {
          return std::nullopt;
        }
        const auto it = slot_values.find(*target);
        if (it == slot_values.end()) {
          return std::nullopt;
        }
        values[load->result] = it->second;
        continue;
      }
      if (load->type_str != "ptr") {
        return std::nullopt;
      }
      const auto target = resolve_pointer_target(load->ptr);
      if (!target.has_value()) {
        return std::nullopt;
      }
      pointer_values[load->result] = *target;
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

std::optional<std::int64_t> parse_minimal_goto_only_constant_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      function.name != "main" ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || !function.alloca_insts.empty() || function.blocks.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    if (!block.insts.empty() || block.label.empty()) {
      return std::nullopt;
    }
    if (!blocks_by_label.emplace(block.label, &block).second) {
      return std::nullopt;
    }

    const auto* br = std::get_if<LirBr>(&block.terminator);
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    if (br == nullptr && ret == nullptr) {
      return std::nullopt;
    }
    if (ret != nullptr) {
      if (!ret->value_str.has_value() || ret->type_str != "i32" ||
          !parse_i64(*ret->value_str).has_value()) {
        return std::nullopt;
      }
    }
  }

  const auto* current = &function.blocks.front();
  if (current->label != "entry") {
    return std::nullopt;
  }

  std::unordered_set<std::string_view> visited;
  while (true) {
    if (!visited.insert(current->label).second) {
      return std::nullopt;
    }
    if (const auto* ret = std::get_if<LirRet>(&current->terminator)) {
      return parse_i64(*ret->value_str);
    }
    const auto* br = std::get_if<LirBr>(&current->terminator);
    if (br == nullptr) {
      return std::nullopt;
    }
    const auto next_it = blocks_by_label.find(br->target_label);
    if (next_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    current = next_it->second;
  }
}

std::optional<std::int64_t> parse_minimal_double_indirect_local_pointer_conditional_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      function.name != "main" ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.empty()) {
    return std::nullopt;
  }

  // Abstract value: integer scalar, floating-point scalar, or pointer alias.
  struct AbsVal {
    bool is_ptr = false;
    bool is_fp = false;
    std::int64_t ival = 0;
    double fval = 0.0;
    std::string ptr_target;  // alloca name when is_ptr
  };

  // Initialize alloca slots.
  std::unordered_map<std::string, AbsVal> slots;
  for (const auto& inst : function.alloca_insts) {
    const auto* a = std::get_if<LirAllocaOp>(&inst);
    if (a == nullptr) return std::nullopt;
    slots[a->result] = AbsVal{};
  }

  // SSA temporary values.
  std::unordered_map<std::string, AbsVal> temps;

  auto resolve = [&](const std::string& name) -> std::optional<AbsVal> {
    if (auto it = temps.find(name); it != temps.end()) return it->second;
    if (auto v = parse_i64(name); v.has_value()) return AbsVal{false, false, *v, 0.0, {}};
    return std::nullopt;
  };

  // Build block index.
  std::unordered_map<std::string_view, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    if (block.label.empty()) return std::nullopt;
    if (!blocks_by_label.emplace(block.label, &block).second) return std::nullopt;
  }

  const auto* current = &function.blocks.front();
  if (current->label != "entry") return std::nullopt;

  std::string predecessor_label;  // tracks the label of the block we branched from

  std::size_t step_limit = 200;
  while (step_limit-- > 0) {
    // Interpret instructions.
    for (const auto& inst : current->insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        auto it = slots.find(store->ptr);
        if (it == slots.end()) {
          // Store through a pointer temp (indirect store).
          auto ptr_val = resolve(store->ptr);
          if (!ptr_val || !ptr_val->is_ptr) return std::nullopt;
          it = slots.find(ptr_val->ptr_target);
          if (it == slots.end()) return std::nullopt;
        }
        // Value is either a literal, a temp, or an alloca name (address-of).
        if (auto sit = slots.find(store->val); sit != slots.end()) {
          // Storing the address of an alloca.
          it->second = AbsVal{true, false, 0, 0.0, store->val};
        } else {
          auto val = resolve(store->val);
          if (!val) return std::nullopt;
          it->second = *val;
        }
      } else if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        // Direct load from alloca or through a pointer temp.
        auto sit = slots.find(load->ptr);
        if (sit != slots.end()) {
          temps[load->result] = sit->second;
        } else {
          auto ptr_val = resolve(load->ptr);
          if (!ptr_val || !ptr_val->is_ptr) return std::nullopt;
          sit = slots.find(ptr_val->ptr_target);
          if (sit == slots.end()) return std::nullopt;
          temps[load->result] = sit->second;
        }
      } else if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
        auto lhs = resolve(cmp->lhs);
        auto rhs = resolve(cmp->rhs);
        if (!lhs || !rhs || lhs->is_ptr || rhs->is_ptr) return std::nullopt;
        bool result = false;
        std::string_view pred = cmp->predicate;
        if (cmp->is_float) {
          if (!lhs->is_fp || !rhs->is_fp) return std::nullopt;
          double lv = lhs->fval, rv = rhs->fval;
          bool l_nan = std::isnan(lv), r_nan = std::isnan(rv);
          if (pred == "oeq") result = !l_nan && !r_nan && lv == rv;
          else if (pred == "one") result = !l_nan && !r_nan && lv != rv;
          else if (pred == "olt") result = !l_nan && !r_nan && lv < rv;
          else if (pred == "ole") result = !l_nan && !r_nan && lv <= rv;
          else if (pred == "ogt") result = !l_nan && !r_nan && lv > rv;
          else if (pred == "oge") result = !l_nan && !r_nan && lv >= rv;
          else if (pred == "ord") result = !l_nan && !r_nan;
          else if (pred == "uno") result = l_nan || r_nan;
          else if (pred == "ueq") result = l_nan || r_nan || lv == rv;
          else if (pred == "une") result = l_nan || r_nan || lv != rv;
          else if (pred == "ult") result = l_nan || r_nan || lv < rv;
          else if (pred == "ule") result = l_nan || r_nan || lv <= rv;
          else if (pred == "ugt") result = l_nan || r_nan || lv > rv;
          else if (pred == "uge") result = l_nan || r_nan || lv >= rv;
          else return std::nullopt;
        } else {
          if (lhs->is_fp || rhs->is_fp) return std::nullopt;
          if (pred == "eq") result = (lhs->ival == rhs->ival);
          else if (pred == "ne") result = (lhs->ival != rhs->ival);
          else if (pred == "slt") result = (lhs->ival < rhs->ival);
          else if (pred == "sle") result = (lhs->ival <= rhs->ival);
          else if (pred == "sgt") result = (lhs->ival > rhs->ival);
          else if (pred == "sge") result = (lhs->ival >= rhs->ival);
          else if (pred == "ult") result = (static_cast<std::uint64_t>(lhs->ival) < static_cast<std::uint64_t>(rhs->ival));
          else if (pred == "ule") result = (static_cast<std::uint64_t>(lhs->ival) <= static_cast<std::uint64_t>(rhs->ival));
          else if (pred == "ugt") result = (static_cast<std::uint64_t>(lhs->ival) > static_cast<std::uint64_t>(rhs->ival));
          else if (pred == "uge") result = (static_cast<std::uint64_t>(lhs->ival) >= static_cast<std::uint64_t>(rhs->ival));
          else return std::nullopt;
        }
        temps[cmp->result] = AbsVal{false, false, result ? 1 : 0, 0.0, {}};
      } else if (const auto* cast = std::get_if<LirCastOp>(&inst)) {
        auto src = resolve(cast->operand);
        if (!src || src->is_ptr) return std::nullopt;
        switch (cast->kind) {
          case LirCastKind::ZExt: {
            if (src->is_fp) return std::nullopt;
            temps[cast->result] = AbsVal{false, false, src->ival, 0.0, {}};
            break;
          }
          case LirCastKind::SExt: {
            if (src->is_fp) return std::nullopt;
            std::int64_t val = src->ival;
            int from_bits = 0;
            if (cast->from_type == "i1") from_bits = 1;
            else if (cast->from_type == "i8") from_bits = 8;
            else if (cast->from_type == "i16") from_bits = 16;
            else if (cast->from_type == "i32") from_bits = 32;
            else if (cast->from_type == "i64") from_bits = 64;
            else return std::nullopt;
            if (from_bits < 64) {
              std::int64_t mask = (static_cast<std::int64_t>(1) << from_bits) - 1;
              val &= mask;
              if (val & (static_cast<std::int64_t>(1) << (from_bits - 1))) {
                val |= ~mask;
              }
            }
            temps[cast->result] = AbsVal{false, false, val, 0.0, {}};
            break;
          }
          case LirCastKind::Trunc: {
            if (src->is_fp) return std::nullopt;
            std::int64_t val = src->ival;
            int to_bits = 0;
            if (cast->to_type == "i1") to_bits = 1;
            else if (cast->to_type == "i8") to_bits = 8;
            else if (cast->to_type == "i16") to_bits = 16;
            else if (cast->to_type == "i32") to_bits = 32;
            else if (cast->to_type == "i64") to_bits = 64;
            else return std::nullopt;
            if (to_bits < 64) {
              val &= (static_cast<std::int64_t>(1) << to_bits) - 1;
            }
            temps[cast->result] = AbsVal{false, false, val, 0.0, {}};
            break;
          }
          case LirCastKind::SIToFP: {
            if (src->is_fp) return std::nullopt;
            temps[cast->result] = AbsVal{false, true, 0, static_cast<double>(src->ival), {}};
            break;
          }
          case LirCastKind::UIToFP: {
            if (src->is_fp) return std::nullopt;
            temps[cast->result] = AbsVal{false, true, 0, static_cast<double>(static_cast<std::uint64_t>(src->ival)), {}};
            break;
          }
          case LirCastKind::FPToSI: {
            if (!src->is_fp) return std::nullopt;
            temps[cast->result] = AbsVal{false, false, static_cast<std::int64_t>(src->fval), 0.0, {}};
            break;
          }
          case LirCastKind::FPToUI: {
            if (!src->is_fp) return std::nullopt;
            temps[cast->result] = AbsVal{false, false, static_cast<std::int64_t>(static_cast<std::uint64_t>(src->fval)), 0.0, {}};
            break;
          }
          case LirCastKind::FPExt:
          case LirCastKind::FPTrunc: {
            if (!src->is_fp) return std::nullopt;
            double val = src->fval;
            if (cast->kind == LirCastKind::FPTrunc &&
                cast->to_type == "float") {
              val = static_cast<double>(static_cast<float>(val));
            }
            temps[cast->result] = AbsVal{false, true, 0, val, {}};
            break;
          }
          default: return std::nullopt;
        }
      } else if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
        auto lhs = resolve(bin->lhs);
        auto rhs = resolve(bin->rhs);
        if (!lhs || !rhs || lhs->is_ptr || rhs->is_ptr) return std::nullopt;
        if (lhs->is_fp || rhs->is_fp) return std::nullopt;
        auto typed_op = bin->opcode.typed();
        if (!typed_op) return std::nullopt;
        std::int64_t val = 0;
        switch (*typed_op) {
          case LirBinaryOpcode::Add: val = lhs->ival + rhs->ival; break;
          case LirBinaryOpcode::Sub: val = lhs->ival - rhs->ival; break;
          case LirBinaryOpcode::Mul: val = lhs->ival * rhs->ival; break;
          case LirBinaryOpcode::SDiv:
            if (rhs->ival == 0) return std::nullopt;
            val = lhs->ival / rhs->ival;
            break;
          case LirBinaryOpcode::SRem:
            if (rhs->ival == 0) return std::nullopt;
            val = lhs->ival % rhs->ival;
            break;
          case LirBinaryOpcode::And: val = lhs->ival & rhs->ival; break;
          case LirBinaryOpcode::Or:  val = lhs->ival | rhs->ival; break;
          case LirBinaryOpcode::Xor: val = lhs->ival ^ rhs->ival; break;
          case LirBinaryOpcode::Shl: val = lhs->ival << rhs->ival; break;
          case LirBinaryOpcode::LShr:
            val = static_cast<std::int64_t>(
                static_cast<std::uint64_t>(lhs->ival) >> rhs->ival);
            break;
          case LirBinaryOpcode::AShr: val = lhs->ival >> rhs->ival; break;
          default: return std::nullopt;
        }
        temps[bin->result] = AbsVal{false, false, val, 0.0, {}};
      } else if (const auto* sel = std::get_if<LirSelectOp>(&inst)) {
        auto cond = resolve(sel->cond);
        if (!cond || cond->is_ptr) return std::nullopt;
        auto tv = resolve(sel->true_val);
        auto fv = resolve(sel->false_val);
        if (!tv || !fv || tv->is_ptr || fv->is_ptr) return std::nullopt;
        temps[sel->result] = (cond->ival != 0) ? *tv : *fv;
      } else if (const auto* phi = std::get_if<LirPhiOp>(&inst)) {
        if (predecessor_label.empty()) return std::nullopt;
        bool found = false;
        for (const auto& [val, label] : phi->incoming) {
          if (label == predecessor_label) {
            auto v = resolve(val);
            if (!v) return std::nullopt;
            temps[phi->result] = *v;
            found = true;
            break;
          }
        }
        if (!found) return std::nullopt;
      } else {
        return std::nullopt;
      }
    }

    // Interpret terminator.
    if (const auto* ret = std::get_if<LirRet>(&current->terminator)) {
      if (!ret->value_str.has_value() || ret->type_str != "i32") return std::nullopt;
      auto val = resolve(*ret->value_str);
      if (!val || val->is_ptr || val->is_fp) return std::nullopt;
      return val->ival;
    }
    if (const auto* br = std::get_if<LirBr>(&current->terminator)) {
      auto it = blocks_by_label.find(br->target_label);
      if (it == blocks_by_label.end()) return std::nullopt;
      predecessor_label = current->label;
      current = it->second;
      continue;
    }
    if (const auto* cbr = std::get_if<LirCondBr>(&current->terminator)) {
      auto cond = resolve(cbr->cond_name);
      if (!cond || cond->is_ptr) return std::nullopt;
      const auto& target = (cond->ival != 0) ? cbr->true_label : cbr->false_label;
      auto it = blocks_by_label.find(target);
      if (it == blocks_by_label.end()) return std::nullopt;
      predecessor_label = current->label;
      current = it->second;
      continue;
    }
    return std::nullopt;
  }
  return std::nullopt;
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
                                                 c4c::backend::BackendBinaryOpcode opcode) {
  if (opcode == c4c::backend::BackendBinaryOpcode::Add) {
    if ((lhs.uses_first_param && rhs.uses_first_param) ||
        (lhs.uses_second_param && rhs.uses_second_param)) {
      return std::nullopt;
    }
    return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                       lhs.uses_second_param || rhs.uses_second_param,
                       lhs.constant + rhs.constant};
  }
  if (rhs.uses_first_param || rhs.uses_second_param) {
    return std::nullopt;
  }
  return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                     lhs.constant - rhs.constant};
}

std::optional<AffineValue> combine_affine_values(const AffineValue& lhs,
                                                 const AffineValue& rhs,
                                                 c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return combine_affine_values(lhs, rhs, c4c::backend::BackendBinaryOpcode::Add);
    case c4c::backend::bir::BinaryOpcode::Sub:
      return combine_affine_values(lhs, rhs, c4c::backend::BackendBinaryOpcode::Sub);
  }
  return std::nullopt;
}

const c4c::backend::bir::BinaryInst* get_binary_inst(
    const c4c::backend::bir::Inst& inst) {
  return std::get_if<c4c::backend::bir::BinaryInst>(&inst);
}

bool is_i32_scalar_signature_return(const c4c::backend::BackendFunctionSignature& signature) {
  return c4c::backend::backend_signature_return_type_kind(signature) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_signature_return_scalar_type(signature) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_param(const c4c::backend::BackendParam& param) {
  return c4c::backend::backend_param_type_kind(param) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_param_scalar_type(param) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_call_return(const c4c::backend::BackendCallInst& call) {
  return c4c::backend::backend_call_return_type_kind(call) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_call_return_scalar_type(call) ==
             c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_binary(const c4c::backend::BackendBinaryInst& inst) {
  return c4c::backend::backend_binary_value_type(inst) ==
         c4c::backend::BackendScalarType::I32;
}

bool is_i32_scalar_global(const c4c::backend::BackendGlobal& global) {
  return c4c::backend::backend_global_value_type_kind(global) ==
             c4c::backend::BackendValueTypeKind::Scalar &&
         c4c::backend::backend_global_scalar_type(global) ==
             c4c::backend::BackendScalarType::I32;
}

const c4c::backend::BackendLocalSlot* find_local_slot(
    const c4c::backend::BackendFunction& function,
    std::string_view slot_name) {
  for (const auto& slot : function.local_slots) {
    if (slot.name == slot_name) {
      return &slot;
    }
  }
  return nullptr;
}

bool is_two_element_i32_local_array_slot(const c4c::backend::BackendFunction& function,
                                         std::string_view slot_name) {
  const auto* slot = find_local_slot(function, slot_name);
  return slot != nullptr && slot->size_bytes == 8 &&
         slot->element_type == c4c::backend::BackendScalarType::I32 &&
         slot->element_size_bytes == 4;
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
  return asm_symbol_name(module.target_triple, logical_name);
}

std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name) {
  const bool is_darwin = target_triple.find("apple-darwin") != std::string::npos;
  if (!is_darwin) return std::string(logical_name);
  return std::string("_") + std::string(logical_name);
}

std::string asm_private_data_label(const c4c::backend::BackendModule& module,
                                   std::string_view pool_name) {
  return asm_private_data_label(module.target_triple, pool_name);
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

std::string decode_llvm_c_string(std::string_view bytes) {
  auto hex_value = [](unsigned char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };

  std::string decoded;
  decoded.reserve(bytes.size());
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const char ch = bytes[i];
    if (ch != '\\' || i + 1 >= bytes.size()) {
      decoded.push_back(ch);
      continue;
    }

    const char code = bytes[i + 1];
    const int hi = hex_value(static_cast<unsigned char>(code));
    if (code == 'x' && i + 3 < bytes.size()) {
      const int hi2 = hex_value(static_cast<unsigned char>(bytes[i + 2]));
      const int lo2 = hex_value(static_cast<unsigned char>(bytes[i + 3]));
      if (hi2 >= 0 && lo2 >= 0) {
        decoded.push_back(static_cast<char>((hi2 << 4) | lo2));
        i += 3;
        continue;
      }
      decoded.push_back('\\');
      continue;
    }

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
}

std::string escape_asm_string(std::string_view bytes) {
  const std::string decoded = decode_llvm_c_string(bytes);

  static constexpr char kHexDigits[] = "0123456789abcdef";

  std::string escaped;
  escaped.reserve(decoded.size());
  for (unsigned char ch : decoded) {
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

const c4c::codegen::lir::LirStringConst* find_string_constant(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  for (const auto& string_constant : module.string_pool) {
    if (string_constant.pool_name == name) return &string_constant;
    if (!name.empty() && name.front() != '@' && string_constant.pool_name == ("@" + std::string(name))) {
      return &string_constant;
    }
  }
  return nullptr;
}

std::optional<std::int64_t> parse_single_block_return_imm(
    const c4c::backend::BackendFunction& function) {
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      !is_i32_scalar_signature_return(function.signature) ||
      !function.signature.params.empty() ||
      function.signature.is_vararg || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32 ||
      !block.insts.empty()) {
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

c4c::backend::RegAllocIntegrationResult synthesize_shared_x86_call_crossing_regalloc(
    const MinimalCallCrossingDirectCallSlice& slice) {
  c4c::backend::RegAllocIntegrationResult regalloc;
  const std::string& source_value = slice.regalloc_source_value.empty()
                                        ? synthetic_call_crossing_regalloc_source_value()
                                        : slice.regalloc_source_value;
  regalloc.reg_assignments.emplace(source_value,
                                   c4c::backend::PhysReg{1});
  regalloc.used_callee_saved.push_back(c4c::backend::PhysReg{1});
  return regalloc;
}

bool is_minimal_single_function_asm_slice(const c4c::backend::BackendModule& module) {
  const c4c::backend::BackendFunction* function = nullptr;
  for (const auto& candidate : module.functions) {
    if (candidate.is_declaration) continue;
    if (function != nullptr) return false;
    function = &candidate;
  }
  if (function == nullptr || !backend_function_is_definition(function->signature) ||
      !is_i32_scalar_signature_return(function->signature) || function->signature.is_vararg ||
      function->blocks.size() != 1) {
    return false;
  }

  const auto& block = function->blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return false;
  }

  return true;
}

const c4c::backend::BackendFunction* minimal_single_backend_function(
    const c4c::backend::BackendModule& module) {
  for (const auto& function : module.functions) {
    if (!function.is_declaration) {
      return &function;
    }
  }
  return nullptr;
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

std::string minimal_single_function_symbol(const c4c::backend::BackendModule& module) {
  for (const auto& function : module.functions) {
    if (!function.is_declaration) {
      return asm_symbol_name(module, function.signature.name);
    }
  }

  return asm_symbol_name(module, module.functions.front().signature.name);
}

std::string minimal_single_function_symbol(const c4c::backend::bir::Module& module) {
  return asm_symbol_name(module.target_triple, module.functions.front().name);
}

std::optional<std::int64_t> parse_minimal_return_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto* function = minimal_single_backend_function(module);
  if (function == nullptr) {
    return std::nullopt;
  }
  const auto& block = function->blocks.front();
  if (!block.insts.empty()) {
    return std::nullopt;
  }

  return parse_i64(*block.terminator.value);
}

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
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto* function = minimal_single_backend_function(module);
  if (function == nullptr) {
    return std::nullopt;
  }
  const auto& block = function->blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      !is_i32_scalar_binary(*add) || *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(add->lhs);
  const auto rhs = parse_i64(add->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs + *rhs;
}

std::optional<std::int64_t> parse_minimal_return_add_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* add = get_binary_inst(block.insts.front());
  if (add == nullptr || add->opcode != c4c::backend::bir::BinaryOpcode::Add ||
      add->result.kind != c4c::backend::bir::Value::Kind::Named ||
      add->result.type != c4c::backend::bir::TypeKind::I32 ||
      !block.terminator.value.has_value() ||
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
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto* function = minimal_single_backend_function(module);
  if (function == nullptr) {
    return std::nullopt;
  }
  const auto& block = function->blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* sub = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::BackendBinaryOpcode::Sub ||
      !is_i32_scalar_binary(*sub) || *block.terminator.value != sub->result) {
    return std::nullopt;
  }

  const auto lhs = parse_i64(sub->lhs);
  const auto rhs = parse_i64(sub->rhs);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  return *lhs - *rhs;
}

std::optional<std::int64_t> parse_minimal_return_sub_imm(
    const c4c::backend::bir::Module& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* sub = get_binary_inst(block.insts.front());
  if (sub == nullptr || sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->result.type != c4c::backend::bir::TypeKind::I32 ||
      !block.terminator.value.has_value() ||
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

std::optional<MinimalAffineReturnSlice> parse_minimal_affine_return_slice(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto* function = minimal_single_backend_function(module);
  if (function == nullptr || function->signature.params.size() > 2 ||
      function->signature.params.empty()) {
    return std::nullopt;
  }
  for (const auto& param : function->signature.params) {
    if (!is_i32_scalar_param(param)) {
      return std::nullopt;
    }
  }

  std::vector<std::string_view> param_names;
  param_names.reserve(function->signature.params.size());
  for (const auto& param : function->signature.params) {
    param_names.push_back(param.name);
  }
  const auto& block = function->blocks.front();
  std::unordered_map<std::string, AffineValue> values;
  for (const auto& inst : block.insts) {
    const auto* bin = std::get_if<c4c::backend::BackendBinaryInst>(&inst);
    if (bin == nullptr || !is_i32_scalar_binary(*bin) || bin->result.empty()) {
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
    values[bin->result] = *combined;
  }

  const auto lowered_return =
      lower_affine_operand(*block.terminator.value, param_names, values);
  if (!lowered_return.has_value()) {
    return std::nullopt;
  }

  return MinimalAffineReturnSlice{
      function->signature.name,
      function->signature.params.size(),
      lowered_return->uses_first_param,
      lowered_return->uses_second_param,
      lowered_return->constant,
  };
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

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
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
      c4c::backend::backend_return_scalar_type(true_block.terminator) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_return_scalar_type(false_block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  if (cmp == nullptr || cmp->result != entry.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32) {
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

  return MinimalConditionalReturnSlice{
      "main",
      0,
      predicate,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *lhs_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *rhs_imm},
      true_block.label,
      false_block.label,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *true_return_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *false_return_imm},
  };
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1) return std::nullopt;

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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

  return MinimalConditionalReturnSlice{
      "main",
      0,
      *cmp0_predicate,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *lhs_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *rhs_imm},
      condbr->true_label,
      condbr->false_label,
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *true_return_imm},
      {MinimalConditionalReturnSlice::ValueKind::Immediate, *false_return_imm},
  };
}

std::optional<MinimalCountdownLoopSlice> parse_minimal_countdown_loop_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
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
      !exit.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(exit.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&loop.insts[0]);
  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&loop.insts[1]);
  const auto* dec = std::get_if<c4c::backend::BackendBinaryInst>(&body.insts.front());
  if (phi == nullptr || cmp == nullptr || dec == nullptr ||
      c4c::backend::backend_phi_value_type(*phi) != c4c::backend::BackendScalarType::I32 ||
      phi->incoming.size() != 2 || phi->incoming[0].label != entry.label ||
      phi->incoming[1].label != body.label || cmp->predicate != c4c::backend::BackendComparePredicate::Ne ||
      cmp->result != loop.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32 ||
      cmp->lhs != phi->result || cmp->rhs != "0" ||
      dec->opcode != c4c::backend::BackendBinaryOpcode::Sub ||
      c4c::backend::backend_binary_value_type(*dec) != c4c::backend::BackendScalarType::I32 ||
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

std::optional<MinimalCountdownLoopSlice> parse_minimal_countdown_loop_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 4 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->type_str != "i32") {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& loop = function.blocks[1];
  const auto& body = function.blocks[2];
  const auto& exit = function.blocks[3];

  const auto* entry_store =
      entry.insts.empty() ? nullptr : std::get_if<LirStoreOp>(&entry.insts.front());
  const auto* entry_branch = std::get_if<LirBr>(&entry.terminator);
  const auto* loop_load =
      loop.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&loop.insts[0]);
  const auto* loop_cmp =
      loop.insts.size() < 2 ? nullptr : std::get_if<LirCmpOp>(&loop.insts[1]);
  const auto* loop_branch = std::get_if<LirCondBr>(&loop.terminator);
  const auto* body_load =
      body.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&body.insts[0]);
  const auto* body_sub =
      body.insts.size() < 2 ? nullptr : std::get_if<LirBinOp>(&body.insts[1]);
  const auto* body_store =
      body.insts.size() < 3 ? nullptr : std::get_if<LirStoreOp>(&body.insts[2]);
  const auto* body_branch = std::get_if<LirBr>(&body.terminator);
  const auto* exit_load =
      exit.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&exit.insts[0]);
  const auto* exit_ret = std::get_if<LirRet>(&exit.terminator);
  const auto cmp_predicate = loop_cmp == nullptr ? std::nullopt : loop_cmp->predicate.typed();
  const auto sub_opcode = body_sub == nullptr ? std::nullopt : body_sub->opcode.typed();
  if (entry.label != "entry" || entry.insts.size() != 1 || entry_store == nullptr ||
      entry_branch == nullptr || entry_branch->target_label != loop.label ||
      entry_store->type_str != "i32" || entry_store->ptr != slot->result ||
      loop.insts.size() != 2 || loop_load == nullptr || loop_cmp == nullptr ||
      loop_branch == nullptr || loop_load->type_str != "i32" || loop_load->ptr != slot->result ||
      loop_cmp->is_float || cmp_predicate != LirCmpPredicate::Ne ||
      loop_cmp->type_str != "i32" || loop_cmp->lhs != loop_load->result ||
      loop_cmp->rhs != "0" || loop_branch->cond_name != loop_cmp->result ||
      loop_branch->true_label != body.label || loop_branch->false_label != exit.label ||
      body.insts.size() != 3 || body_load == nullptr || body_sub == nullptr ||
      body_store == nullptr || body_branch == nullptr ||
      body_load->type_str != "i32" || body_load->ptr != slot->result ||
      sub_opcode != LirBinaryOpcode::Sub || body_sub->type_str != "i32" ||
      body_sub->lhs != body_load->result || body_sub->rhs != "1" ||
      body_store->type_str != "i32" || body_store->val != body_sub->result ||
      body_store->ptr != slot->result || body_branch->target_label != loop.label ||
      exit.insts.size() != 1 || exit_load == nullptr || exit_ret == nullptr ||
      exit_load->type_str != "i32" || exit_load->ptr != slot->result ||
      exit_ret->type_str != "i32" || exit_ret->value_str != exit_load->result) {
    return std::nullopt;
  }

  const auto initial_imm = parse_i64(entry_store->val);
  if (!initial_imm.has_value() || *initial_imm < 0) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{*initial_imm, loop.label, body.label, exit.label};
}

std::optional<std::int64_t> parse_minimal_countdown_do_while_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 5 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->type_str != "i32") {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& body = function.blocks[1];
  const auto& bridge = function.blocks[2];
  const auto& cond = function.blocks[3];
  const auto& exit = function.blocks[4];

  const auto* entry_store =
      entry.insts.empty() ? nullptr : std::get_if<LirStoreOp>(&entry.insts.front());
  const auto* entry_branch = std::get_if<LirBr>(&entry.terminator);
  const auto* body_load =
      body.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&body.insts[0]);
  const auto* body_sub =
      body.insts.size() < 2 ? nullptr : std::get_if<LirBinOp>(&body.insts[1]);
  const auto* body_store =
      body.insts.size() < 3 ? nullptr : std::get_if<LirStoreOp>(&body.insts[2]);
  const auto* body_branch = std::get_if<LirBr>(&body.terminator);
  const auto* bridge_branch = std::get_if<LirBr>(&bridge.terminator);
  const auto* cond_load =
      cond.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&cond.insts[0]);
  const auto* cond_cmp =
      cond.insts.size() < 2 ? nullptr : std::get_if<LirCmpOp>(&cond.insts[1]);
  const auto* cond_branch = std::get_if<LirCondBr>(&cond.terminator);
  const auto* exit_load =
      exit.insts.empty() ? nullptr : std::get_if<LirLoadOp>(&exit.insts[0]);
  const auto* exit_ret = std::get_if<LirRet>(&exit.terminator);
  const auto sub_opcode = body_sub == nullptr ? std::nullopt : body_sub->opcode.typed();
  const auto cmp_predicate = cond_cmp == nullptr ? std::nullopt : cond_cmp->predicate.typed();
  if (entry.label != "entry" || entry.insts.size() != 1 || entry_store == nullptr ||
      entry_branch == nullptr || entry_branch->target_label != body.label ||
      entry_store->type_str != "i32" || entry_store->ptr != slot->result ||
      body.insts.size() != 3 || body_load == nullptr || body_sub == nullptr ||
      body_store == nullptr || body_branch == nullptr ||
      body_load->type_str != "i32" || body_load->ptr != slot->result ||
      sub_opcode != LirBinaryOpcode::Sub || body_sub->type_str != "i32" ||
      body_sub->lhs != body_load->result || body_sub->rhs != "1" ||
      body_store->type_str != "i32" || body_store->val != body_sub->result ||
      body_store->ptr != slot->result || body_branch->target_label != bridge.label ||
      !bridge.insts.empty() || bridge_branch == nullptr ||
      bridge_branch->target_label != cond.label || cond.insts.size() != 2 ||
      cond_load == nullptr || cond_cmp == nullptr || cond_branch == nullptr ||
      cond_load->type_str != "i32" || cond_load->ptr != slot->result ||
      cond_cmp->is_float || cmp_predicate != LirCmpPredicate::Ne ||
      cond_cmp->type_str != "i32" || cond_cmp->lhs != cond_load->result ||
      cond_cmp->rhs != "0" || cond_branch->cond_name != cond_cmp->result ||
      cond_branch->true_label != body.label || cond_branch->false_label != exit.label ||
      exit.insts.size() != 1 || exit_load == nullptr || exit_ret == nullptr ||
      exit_load->type_str != "i32" || exit_load->ptr != slot->result ||
      exit_ret->type_str != "i32" || exit_ret->value_str != exit_load->result) {
    return std::nullopt;
  }

  const auto initial_imm = parse_i64(entry_store->val);
  if (!initial_imm.has_value() || *initial_imm <= 0) {
    return std::nullopt;
  }

  return 0;
}

std::optional<MinimalConditionalPhiJoinSlice> parse_minimal_conditional_phi_join_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 4 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  const auto& join_block = function.blocks[3];

  const auto* cmp0 = entry.insts.empty() ? nullptr : std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = entry.insts.size() < 2 ? nullptr : std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = entry.insts.size() < 3 ? nullptr : std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  const auto cmp0_predicate = cmp0 == nullptr ? std::nullopt : cmp0->predicate.typed();
  const auto cmp1_predicate = cmp1 == nullptr ? std::nullopt : cmp1->predicate.typed();
  if (entry.label != "entry" || entry.insts.size() != 3 || cmp0 == nullptr || cast == nullptr ||
      cmp1 == nullptr || condbr == nullptr || cmp0->is_float || !cmp0_predicate.has_value() ||
      (*cmp0_predicate != LirCmpPredicate::Slt && *cmp0_predicate != LirCmpPredicate::Sle &&
       *cmp0_predicate != LirCmpPredicate::Sgt && *cmp0_predicate != LirCmpPredicate::Sge &&
       *cmp0_predicate != LirCmpPredicate::Eq && *cmp0_predicate != LirCmpPredicate::Ne &&
       *cmp0_predicate != LirCmpPredicate::Ult && *cmp0_predicate != LirCmpPredicate::Ule &&
       *cmp0_predicate != LirCmpPredicate::Ugt && *cmp0_predicate != LirCmpPredicate::Uge) ||
      cmp0->type_str != "i32" || cast->kind != LirCastKind::ZExt ||
      cast->from_type != "i1" || cast->operand != cmp0->result || cast->to_type != "i32" ||
      cmp1->is_float || cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" ||
      cmp1->lhs != cast->result || cmp1->rhs != "0" || condbr->cond_name != cmp1->result ||
      true_block.label != condbr->true_label || false_block.label != condbr->false_label) {
    return std::nullopt;
  }

  const auto* true_branch = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_branch = std::get_if<LirBr>(&false_block.terminator);
  if (true_branch == nullptr || false_branch == nullptr ||
      true_branch->target_label != join_block.label ||
      false_branch->target_label != join_block.label ||
      (join_block.insts.size() != 1 && join_block.insts.size() != 2)) {
    return std::nullopt;
  }

  const auto parse_incoming_value =
      [](const LirBlock& block,
         std::string_view expected_result)
      -> std::optional<MinimalConditionalPhiJoinSlice::IncomingValue> {
    MinimalConditionalPhiJoinSlice::IncomingValue value;
    std::string current_name;

    if (block.insts.empty()) {
      const auto imm = parse_i64(expected_result);
      if (!imm.has_value()) {
        return std::nullopt;
      }
      value.base_imm = *imm;
      return value;
    }

    for (std::size_t index = 0; index < block.insts.size(); ++index) {
      const auto* inst = std::get_if<LirBinOp>(&block.insts[index]);
      const auto opcode = inst == nullptr ? std::nullopt : inst->opcode.typed();
      if (inst == nullptr || !opcode.has_value() || inst->type_str != "i32" ||
          (*opcode != LirBinaryOpcode::Add && *opcode != LirBinaryOpcode::Sub)) {
        return std::nullopt;
      }

      const auto rhs_imm = parse_i64(inst->rhs);
      if (!rhs_imm.has_value()) {
        return std::nullopt;
      }

      if (index == 0) {
        const auto base_imm = parse_i64(inst->lhs);
        if (!base_imm.has_value()) {
          return std::nullopt;
        }
        value.base_imm = *base_imm;
      } else if (inst->lhs != current_name) {
        return std::nullopt;
      }

      value.steps.push_back(
          {*opcode == LirBinaryOpcode::Add ? c4c::backend::BackendBinaryOpcode::Add
                                           : c4c::backend::BackendBinaryOpcode::Sub,
           *rhs_imm});
      current_name = inst->result;
    }

    if (current_name != expected_result) {
      return std::nullopt;
    }

    return value;
  };

  const auto lhs_imm = parse_i64(cmp0->lhs);
  const auto rhs_imm = parse_i64(cmp0->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto* phi = std::get_if<LirPhiOp>(&join_block.insts.front());
  if (phi == nullptr || phi->type_str != "i32" || phi->incoming.size() != 2 ||
      phi->incoming[0].second != true_block.label ||
      phi->incoming[1].second != false_block.label) {
    return std::nullopt;
  }

  std::optional<std::int64_t> join_add_imm;
  if (join_block.insts.size() == 1) {
    const auto* ret = std::get_if<LirRet>(&join_block.terminator);
    if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32" ||
        *ret->value_str != phi->result) {
      return std::nullopt;
    }
  } else {
    const auto* add = std::get_if<LirBinOp>(&join_block.insts[1]);
    const auto* ret = std::get_if<LirRet>(&join_block.terminator);
    const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
    if (add == nullptr || !add_opcode.has_value() || *add_opcode != LirBinaryOpcode::Add ||
        add->type_str != "i32" || add->lhs != phi->result || ret == nullptr ||
        !ret->value_str.has_value() || ret->type_str != "i32" ||
        *ret->value_str != add->result) {
      return std::nullopt;
    }
    join_add_imm = parse_i64(add->rhs);
    if (!join_add_imm.has_value()) {
      return std::nullopt;
    }
  }

  const auto true_value = parse_incoming_value(true_block, phi->incoming[0].first);
  const auto false_value = parse_incoming_value(false_block, phi->incoming[1].first);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  return MinimalConditionalPhiJoinSlice{*cmp0_predicate,
                                        *lhs_imm,
                                        *rhs_imm,
                                        true_block.label,
                                        false_block.label,
                                        join_block.label,
                                        *true_value,
                                        *false_value,
                                        join_add_imm};
}

std::optional<MinimalConditionalPhiJoinSlice> parse_minimal_conditional_phi_join_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      c4c::backend::backend_signature_return_scalar_type(function.signature) !=
          c4c::backend::BackendScalarType::I32 ||
      function.signature.name != "main" ||
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
      return MinimalConditionalPhiJoinSlice::IncomingValue{*imm, {}};
    }
    MinimalConditionalPhiJoinSlice::IncomingValue value;
    std::string previous_result;
    for (std::size_t index = 0; index < block.insts.size(); ++index) {
      const auto* inst = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts[index]);
      if (inst == nullptr ||
          c4c::backend::backend_binary_value_type(*inst) !=
              c4c::backend::BackendScalarType::I32 ||
          (inst->opcode != c4c::backend::BackendBinaryOpcode::Add &&
           inst->opcode != c4c::backend::BackendBinaryOpcode::Sub)) {
        return std::nullopt;
      }
      const auto rhs_imm = parse_i64(inst->rhs);
      if (!rhs_imm.has_value()) {
        return std::nullopt;
      }
      if (index == 0) {
        const auto base_imm = parse_i64(inst->lhs);
        if (!base_imm.has_value()) {
          return std::nullopt;
        }
        value.base_imm = *base_imm;
      } else if (inst->lhs != previous_result) {
        return std::nullopt;
      }
      value.steps.push_back({inst->opcode, *rhs_imm});
      previous_result = inst->result;
    }
    if (previous_result != expected_result) {
      return std::nullopt;
    }
    return value;
  };
  if (entry.label != "entry" || entry.insts.size() != 1 ||
      entry.terminator.kind != c4c::backend::BackendTerminatorKind::CondBranch ||
      true_block.label != entry.terminator.true_label ||
      false_block.label != entry.terminator.false_label ||
      true_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      false_block.terminator.kind != c4c::backend::BackendTerminatorKind::Branch ||
      true_block.terminator.target_label != join_block.label ||
      false_block.terminator.target_label != join_block.label ||
      (join_block.insts.size() != 1 && join_block.insts.size() != 2) ||
      join_block.terminator.kind != c4c::backend::BackendTerminatorKind::Return ||
      !join_block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(join_block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* cmp = std::get_if<c4c::backend::BackendCompareInst>(&entry.insts.front());
  const auto* phi = std::get_if<c4c::backend::BackendPhiInst>(&join_block.insts.front());
  if (cmp == nullptr || phi == nullptr || cmp->result != entry.terminator.cond_name ||
      c4c::backend::backend_compare_operand_type(*cmp) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_phi_value_type(*phi) != c4c::backend::BackendScalarType::I32 ||
      phi->incoming.size() != 2 ||
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
        c4c::backend::backend_binary_value_type(*add) !=
            c4c::backend::BackendScalarType::I32 ||
        add->lhs != phi->result ||
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
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LirLoadOp>(&entry.insts.front()) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || load == nullptr || ret == nullptr ||
      load->type_str != "i32" || load->ptr != ("@" + global.name) ||
      !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{global.name};
}

std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || !c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      c4c::backend::backend_global_linkage(*global) !=
          c4c::backend::BackendGlobalLinkage::External ||
      !global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  const auto element_count = static_cast<std::int64_t>(global_array_type->element_count);
  const auto element_size = static_cast<std::int64_t>(sizeof(std::int32_t));
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name ||
      *block.terminator.value != load->result || load->address.byte_offset < 0 ||
      load->address.byte_offset % element_size != 0 ||
      load->address.byte_offset / element_size >= element_count) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global->name, load->address.byte_offset};
}

std::optional<MinimalLocalArraySlice> parse_minimal_local_array_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 5 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* store0 = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[0]);
  const auto* store1 = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[1]);
  const auto* load0 = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[2]);
  const auto* load1 = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[3]);
  const auto* add = std::get_if<c4c::backend::BackendBinaryInst>(&block.insts[4]);
  if (store0 == nullptr || store1 == nullptr || load0 == nullptr || load1 == nullptr ||
      add == nullptr ||
      c4c::backend::backend_store_value_type(*store0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_store_value_type(*store1) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load1) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load0) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load1) !=
          c4c::backend::BackendScalarType::I32 ||
      add->opcode != c4c::backend::BackendBinaryOpcode::Add ||
      c4c::backend::backend_binary_value_type(*add) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  if (store0->address.base_symbol.empty() ||
      store0->address.base_symbol != store1->address.base_symbol ||
      store0->address.base_symbol != load0->address.base_symbol ||
      store0->address.base_symbol != load1->address.base_symbol ||
      store0->address.byte_offset != 0 || store1->address.byte_offset != 4 ||
      load0->address.byte_offset != 0 || load1->address.byte_offset != 4) {
    return std::nullopt;
  }
  if (store0->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      store1->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      load0->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      load1->address.kind != c4c::backend::BackendAddressBaseKind::LocalSlot ||
      !is_two_element_i32_local_array_slot(*main_fn, store0->address.base_symbol)) {
    return std::nullopt;
  }

  const auto store0_imm = parse_i64(store0->value);
  const auto store1_imm = parse_i64(store1->value);
  if (!store0_imm.has_value() || !store1_imm.has_value() ||
      *block.terminator.value != add->result) {
    return std::nullopt;
  }

  const bool add_matches_loads =
      (add->lhs == load0->result && add->rhs == load1->result) ||
      (add->lhs == load1->result && add->rhs == load0->result);
  if (!add_matches_loads) {
    return std::nullopt;
  }

  return MinimalLocalArraySlice{*store0_imm, *store1_imm};
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || !c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      c4c::backend::backend_global_linkage(*global) !=
          c4c::backend::BackendGlobalLinkage::External ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{global->name};
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }
  bool zero_initializer = false;
  std::int64_t init_imm = 0;
  if (c4c::backend::backend_global_has_zero_initializer(*global)) {
    zero_initializer = true;
  } else if (!c4c::backend::backend_global_has_integer_initializer(*global, &init_imm)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{global->name, init_imm, zero_initializer};
}

std::optional<MinimalGlobalCharPointerDiffSlice> parse_minimal_global_char_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable) {
    return std::nullopt;
  }
  if (!global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I8) {
    return std::nullopt;
  }
  const auto global_size = static_cast<std::int64_t>(global_array_type->element_count);
  if (global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr ||
      c4c::backend::backend_ptrdiff_result_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->lhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->rhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 ||
      c4c::backend::backend_ptrdiff_element_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I8 ||
      ptrdiff->element_size != 1 ||
      ptrdiff->expected_diff != ptrdiff->lhs_address.byte_offset ||
      ptrdiff->lhs_address.byte_offset <= 0 ||
      ptrdiff->lhs_address.byte_offset >= global_size ||
      *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalCharPointerDiffSlice{
      global->name,
      global_size,
      ptrdiff->lhs_address.byte_offset,
  };
}

std::optional<MinimalGlobalIntPointerDiffSlice> parse_minimal_global_int_pointer_diff_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  const auto global_array_type =
      global == nullptr ? std::nullopt : c4c::backend::backend_global_array_type(*global);
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable) {
    return std::nullopt;
  }
  if (!global_array_type.has_value() ||
      global_array_type->element_type_kind != c4c::backend::BackendValueTypeKind::Scalar ||
      global_array_type->element_scalar_type != c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }
  const auto global_size = static_cast<std::int64_t>(global_array_type->element_count);
  if (global_size < 2) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* ptrdiff = std::get_if<c4c::backend::BackendPtrDiffEqInst>(&block.insts.front());
  if (ptrdiff == nullptr ||
      c4c::backend::backend_ptrdiff_result_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->lhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->rhs_address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      ptrdiff->lhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.base_symbol != global->name ||
      ptrdiff->rhs_address.byte_offset != 0 || ptrdiff->expected_diff != 1 ||
      ptrdiff->lhs_address.byte_offset != ptrdiff->element_size ||
      c4c::backend::backend_ptrdiff_element_type(*ptrdiff) !=
          c4c::backend::BackendScalarType::I32 ||
      ptrdiff->element_size != 4 || *block.terminator.value != ptrdiff->result) {
    return std::nullopt;
  }

  return MinimalGlobalIntPointerDiffSlice{global->name, global_size, 4, 2};
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
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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

  return MinimalScalarGlobalLoadSlice{global.name, init_imm, zero_initializer};
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
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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

std::optional<MinimalMultiPrintfVarargSlice> parse_minimal_multi_printf_vararg_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.size() != 2 ||
      !module.globals.empty() || module.extern_decls.size() != 1) {
    return std::nullopt;
  }

  const auto& decl = module.extern_decls.front();
  if (decl.name != "printf" || trim_lir_arg_text(decl.return_type_str) != "i32") {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || entry.insts.size() != 4 || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != "0" || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* gep0 = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* call0 = std::get_if<LirCallOp>(&entry.insts[1]);
  const auto* gep1 = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* call1 = std::get_if<LirCallOp>(&entry.insts[3]);
  if (gep0 == nullptr || call0 == nullptr || gep1 == nullptr || call1 == nullptr) {
    return std::nullopt;
  }

  const auto* string0 = find_string_constant(module, gep0->ptr);
  const auto* string1 = find_string_constant(module, gep1->ptr);
  if (string0 == nullptr || string1 == nullptr || string0->pool_name == string1->pool_name) {
    return std::nullopt;
  }

  const auto expected_type0 = "[" + std::to_string(string0->byte_length) + " x i8]";
  const auto expected_type1 = "[" + std::to_string(string1->byte_length) + " x i8]";
  if (gep0->element_type != expected_type0 || gep1->element_type != expected_type1 ||
      gep0->indices.size() != 2 || gep1->indices.size() != 2 ||
      gep0->indices[0] != "i64 0" || gep0->indices[1] != "i64 0" ||
      gep1->indices[0] != "i64 0" || gep1->indices[1] != "i64 0") {
    return std::nullopt;
  }

  const auto symbol0 = parse_lir_direct_global_callee(call0->callee);
  const auto symbol1 = parse_lir_direct_global_callee(call1->callee);
  const auto call0_param_types = parse_lir_call_param_types(call0->callee_type_suffix);
  const auto call1_param_types = parse_lir_call_param_types(call1->callee_type_suffix);
  const auto call0_args = parse_lir_typed_call_args(call0->args_str);
  const auto call1_args = parse_lir_typed_call_args(call1->args_str);
  if (!symbol0.has_value() || !symbol1.has_value() || !call0_param_types.has_value() ||
      !call1_param_types.has_value() || !call0_args.has_value() || !call1_args.has_value() ||
      *symbol0 != "printf" || *symbol1 != "printf" ||
      call0->return_type != "i32" || call1->return_type != "i32" ||
      call0_param_types->size() != 2 || call1_param_types->size() != 2 ||
      trim_lir_arg_text((*call0_param_types)[0]) != "ptr" ||
      trim_lir_arg_text((*call0_param_types)[1]) != "..." ||
      trim_lir_arg_text((*call1_param_types)[0]) != "ptr" ||
      trim_lir_arg_text((*call1_param_types)[1]) != "..." ||
      call0_args->size() != 1 || call1_args->size() != 1 ||
      trim_lir_arg_text(call0_args->front().type) != "ptr" ||
      trim_lir_arg_text(call1_args->front().type) != "ptr") {
    return std::nullopt;
  }

  if (call0_args->front().operand != gep0->result || call1_args->front().operand != gep1->result) {
    return std::nullopt;
  }

  return MinimalMultiPrintfVarargSlice{string0->pool_name, string1->pool_name};
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
      !c4c::backend::backend_lir_is_zero_arg_i32_main_definition(function.signature_text) ||
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
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->result != *block.terminator.value ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I8 ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::StringConstant ||
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
  if (global == nullptr || c4c::backend::backend_global_is_extern_declaration(*global) ||
      global->storage != c4c::backend::BackendGlobalStorageKind::Mutable ||
      !is_i32_scalar_global(*global)) {
    return std::nullopt;
  }
  std::int64_t init_imm = 0;
  if (!c4c::backend::backend_global_has_integer_initializer(*global, &init_imm)) {
    return std::nullopt;
  }

  const auto* main_fn = find_function(module, "main");
  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !is_i32_scalar_signature_return(main_fn->signature) ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = main_fn->blocks.front();
  if (block.label != "entry" || block.insts.size() != 2 ||
      !block.terminator.value.has_value() ||
      c4c::backend::backend_return_scalar_type(block.terminator) !=
          c4c::backend::BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* store = std::get_if<c4c::backend::BackendStoreInst>(&block.insts[0]);
  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts[1]);
  if (store == nullptr || load == nullptr ||
      c4c::backend::backend_store_value_type(*store) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_value_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      c4c::backend::backend_load_memory_type(*load) !=
          c4c::backend::BackendScalarType::I32 ||
      store->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      load->address.kind != c4c::backend::BackendAddressBaseKind::Global ||
      store->address.base_symbol != global->name ||
      store->address.byte_offset != 0 || load->address.base_symbol != global->name ||
      load->address.byte_offset != 0 || *block.terminator.value != load->result) {
    return std::nullopt;
  }

  const auto store_imm = parse_i64(store->value);
  if (!store_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{global->name, init_imm, *store_imm};
}

std::optional<MinimalCallCrossingDirectCallSlice> parse_minimal_call_crossing_direct_call_slice(
    const c4c::backend::BackendModule& module) {
  const auto parsed = c4c::backend::parse_backend_minimal_call_crossing_direct_call_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr ||
      parsed->regalloc_source_value.empty()) {
    return std::nullopt;
  }

  return MinimalCallCrossingDirectCallSlice{
      parsed->helper->signature.name,
      parsed->main_function == nullptr ? "main" : parsed->main_function->signature.name,
      parsed->source_imm,
      parsed->helper_add_imm,
      std::string(parsed->regalloc_source_value),
  };
}

std::optional<MinimalCallCrossingDirectCallSlice> parse_minimal_call_crossing_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_call_crossing_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->regalloc_source_value.empty()) {
    return std::nullopt;
  }

  return MinimalCallCrossingDirectCallSlice{
      parsed->helper->name,
      parsed->main_function == nullptr ? "main" : parsed->main_function->name,
      parsed->source_imm,
      parsed->helper_add_imm,
      std::string(parsed->regalloc_source_value),
  };
}

std::optional<MinimalTwoArgDirectCallSlice> parse_minimal_two_arg_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = c4c::backend::parse_backend_minimal_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr) {
    return std::nullopt;
  }

  return MinimalTwoArgDirectCallSlice{
      parsed->helper->name,
      parsed->main_function->name,
      parsed->lhs_call_arg_imm,
      parsed->rhs_call_arg_imm,
  };
}

std::optional<std::int64_t> parse_minimal_folded_two_arg_direct_call_return_imm(
    const c4c::backend::BackendModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_folded_two_arg_direct_call_module(module);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return parsed->return_imm;
}

std::optional<std::int64_t> parse_minimal_folded_two_arg_direct_call_return_imm(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_folded_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return parsed->return_imm;
}

std::optional<MinimalNamedReturnImmSlice> parse_minimal_folded_two_arg_direct_call_return_slice(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_folded_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->main_function == nullptr) {
    return std::nullopt;
  }

  return MinimalNamedReturnImmSlice{
      parsed->main_function->name,
      parsed->return_imm,
  };
}

std::optional<MinimalDualIdentityDirectCallSubSlice> parse_minimal_dual_identity_direct_call_sub_slice(
    const c4c::backend::BackendModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_dual_identity_direct_call_sub_module(module);
  if (!parsed.has_value() || parsed->lhs_helper == nullptr || parsed->rhs_helper == nullptr ||
      parsed->main_function == nullptr) {
    return std::nullopt;
  }

  return MinimalDualIdentityDirectCallSubSlice{
      parsed->lhs_helper->signature.name,
      parsed->rhs_helper->signature.name,
      parsed->main_function->signature.name,
      parsed->lhs_call_arg_imm,
      parsed->rhs_call_arg_imm,
  };
}

std::optional<MinimalDualIdentityDirectCallSubSlice> parse_minimal_dual_identity_direct_call_sub_slice(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed =
      c4c::backend::parse_backend_minimal_dual_identity_direct_call_sub_lir_module(module);
  if (!parsed.has_value() || parsed->lhs_helper == nullptr || parsed->rhs_helper == nullptr ||
      parsed->main_function == nullptr) {
    return std::nullopt;
  }

  return MinimalDualIdentityDirectCallSubSlice{
      parsed->lhs_helper->name,
      parsed->rhs_helper->name,
      parsed->main_function->name,
      parsed->lhs_call_arg_imm,
      parsed->rhs_call_arg_imm,
  };
}

std::optional<MinimalMemberArrayRuntimeSlice> parse_minimal_member_array_runtime_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto* main_fn = static_cast<const c4c::codegen::lir::LirFunction*>(nullptr);
  const auto* helper = static_cast<const c4c::codegen::lir::LirFunction*>(nullptr);
  for (const auto& function : module.functions) {
    if (!function.is_declaration &&
        c4c::backend::backend_lir_signature_matches(
            function.signature_text, "define", "i32", function.name, {})) {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
    } else {
      if (helper != nullptr) {
        return std::nullopt;
      }
      helper = &function;
    }
  }
  if (helper == nullptr || main_fn == nullptr || helper->is_declaration || main_fn->is_declaration ||
      helper->entry.value != 0 || main_fn->entry.value != 0 || helper->blocks.size() != 1 ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  MinimalMemberArrayRuntimeSlice slice;
  const auto& helper_block = helper->blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
  if (helper_block.label != "entry" || helper_ret == nullptr ||
      !helper_ret->value_str.has_value() || helper_ret->type_str != "i32") {
    return std::nullopt;
  }

  if (helper->signature_text ==
          ("define i32 @" + helper->name + "(%struct.Pair %p.p)\n") &&
      helper->alloca_insts.size() == 2 && helper_block.insts.size() == 5) {
    const auto* alloca = std::get_if<LirAllocaOp>(&helper->alloca_insts[0]);
    const auto* store = std::get_if<LirStoreOp>(&helper->alloca_insts[1]);
    const auto* field_gep = std::get_if<LirGepOp>(&helper_block.insts[0]);
    const auto* array_gep = std::get_if<LirGepOp>(&helper_block.insts[1]);
    const auto* index_cast = std::get_if<LirCastOp>(&helper_block.insts[2]);
    const auto* elem_gep = std::get_if<LirGepOp>(&helper_block.insts[3]);
    const auto* load = std::get_if<LirLoadOp>(&helper_block.insts[4]);
    if (alloca == nullptr || store == nullptr || field_gep == nullptr || array_gep == nullptr ||
        index_cast == nullptr || elem_gep == nullptr || load == nullptr ||
        alloca->result != "%lv.param.p" || alloca->type_str != "%struct.Pair" ||
        store->type_str != "%struct.Pair" || store->val != "%p.p" || store->ptr != "%lv.param.p" ||
        field_gep->element_type != "%struct.Pair" || field_gep->ptr != "%lv.param.p" ||
        field_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
        array_gep->element_type != "[2 x i32]" || array_gep->ptr != field_gep->result ||
        array_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
        index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
        index_cast->operand != "1" || index_cast->to_type != "i64" ||
        elem_gep->element_type != "i32" || elem_gep->ptr != array_gep->result ||
        elem_gep->indices != std::vector<std::string>{"i64 " + index_cast->result} ||
        load->type_str != "i32" || load->ptr != elem_gep->result ||
        *helper_ret->value_str != load->result) {
      return std::nullopt;
    }
    slice.kind = MinimalMemberArrayRuntimeSlice::Kind::ByValueParam;
  } else if (helper->signature_text ==
                 ("define i32 @" + helper->name + "(%struct.Outer %p.o)\n") &&
             helper->alloca_insts.size() == 2 && helper_block.insts.size() == 6) {
    const auto* alloca = std::get_if<LirAllocaOp>(&helper->alloca_insts[0]);
    const auto* store = std::get_if<LirStoreOp>(&helper->alloca_insts[1]);
    const auto* outer_gep = std::get_if<LirGepOp>(&helper_block.insts[0]);
    const auto* inner_gep = std::get_if<LirGepOp>(&helper_block.insts[1]);
    const auto* array_gep = std::get_if<LirGepOp>(&helper_block.insts[2]);
    const auto* index_cast = std::get_if<LirCastOp>(&helper_block.insts[3]);
    const auto* elem_gep = std::get_if<LirGepOp>(&helper_block.insts[4]);
    const auto* load = std::get_if<LirLoadOp>(&helper_block.insts[5]);
    if (alloca == nullptr || store == nullptr || outer_gep == nullptr || inner_gep == nullptr ||
        array_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
        load == nullptr || alloca->result != "%lv.param.o" || alloca->type_str != "%struct.Outer" ||
        store->type_str != "%struct.Outer" || store->val != "%p.o" || store->ptr != "%lv.param.o" ||
        outer_gep->element_type != "%struct.Outer" || outer_gep->ptr != "%lv.param.o" ||
        outer_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
        inner_gep->element_type != "%struct.Inner" || inner_gep->ptr != outer_gep->result ||
        inner_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
        array_gep->element_type != "[2 x i32]" || array_gep->ptr != inner_gep->result ||
        array_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
        index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
        index_cast->operand != "1" || index_cast->to_type != "i64" ||
        elem_gep->element_type != "i32" || elem_gep->ptr != array_gep->result ||
        elem_gep->indices != std::vector<std::string>{"i64 " + index_cast->result} ||
        load->type_str != "i32" || load->ptr != elem_gep->result ||
        *helper_ret->value_str != load->result) {
      return std::nullopt;
    }
    slice.kind = MinimalMemberArrayRuntimeSlice::Kind::ByValueParam;
  } else if (helper->signature_text ==
                 ("define i32 @" + helper->name + "(ptr %p.o)\n") &&
             helper->alloca_insts.empty() && helper_block.insts.size() == 7) {
    const auto* outer_gep = std::get_if<LirGepOp>(&helper_block.insts[0]);
    const auto* load_inner = std::get_if<LirLoadOp>(&helper_block.insts[1]);
    const auto* inner_gep = std::get_if<LirGepOp>(&helper_block.insts[2]);
    const auto* array_gep = std::get_if<LirGepOp>(&helper_block.insts[3]);
    const auto* index_cast = std::get_if<LirCastOp>(&helper_block.insts[4]);
    const auto* elem_gep = std::get_if<LirGepOp>(&helper_block.insts[5]);
    const auto* load = std::get_if<LirLoadOp>(&helper_block.insts[6]);
    if (outer_gep == nullptr || load_inner == nullptr || inner_gep == nullptr ||
        array_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
        load == nullptr || outer_gep->element_type != "%struct.Outer" ||
        outer_gep->ptr != "%p.o" ||
        outer_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
        load_inner->type_str != "ptr" || load_inner->ptr != outer_gep->result ||
        inner_gep->element_type != "%struct.Inner" || inner_gep->ptr != load_inner->result ||
        inner_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
        array_gep->element_type != "[2 x i32]" || array_gep->ptr != inner_gep->result ||
        array_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
        index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
        index_cast->operand != "1" || index_cast->to_type != "i64" ||
        elem_gep->element_type != "i32" || elem_gep->ptr != array_gep->result ||
        elem_gep->indices != std::vector<std::string>{"i64 " + index_cast->result} ||
        load->type_str != "i32" || load->ptr != elem_gep->result ||
        *helper_ret->value_str != load->result) {
      return std::nullopt;
    }
    slice.kind = MinimalMemberArrayRuntimeSlice::Kind::NestedMemberPointer;
  } else {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
  if (main_block.label != "entry" || main_ret == nullptr || !main_ret->value_str.has_value() ||
      main_ret->type_str != "i32" || main_block.insts.size() < 3) {
    return std::nullopt;
  }

  std::vector<std::int64_t> element_imms;
  for (const auto& inst : main_block.insts) {
    const auto* store = std::get_if<LirStoreOp>(&inst);
    if (store == nullptr || store->type_str != "i32") {
      continue;
    }
    const auto imm = parse_i64(store->val);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    element_imms.push_back(*imm);
  }
  if (element_imms.size() != 2) {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&main_block.insts.back());
  if (call == nullptr || call->return_type != "i32" ||
      call->callee != ("@" + helper->name) ||
      *main_ret->value_str != call->result) {
    return std::nullopt;
  }

  slice.helper_name = helper->name;
  slice.caller_name = main_fn->name;
  slice.first_imm = element_imms[0];
  slice.second_imm = element_imms[1];
  return slice;
}

void emit_function_prelude(std::ostringstream& out,
                           const c4c::backend::BackendModule& module,
                           std::string_view symbol,
                           bool is_global) {
  (void)module;
  if (is_global) out << ".globl " << symbol << "\n";
  out << ".type " << symbol << ", %function\n";
  out << symbol << ":\n";
}

void emit_function_prelude(std::ostringstream& out,
                           std::string_view symbol,
                           bool is_global) {
  if (is_global) out << ".globl " << symbol << "\n";
  out << ".type " << symbol << ", %function\n";
  out << symbol << ":\n";
}

std::string emit_minimal_return_asm(std::string_view target_triple,
                                    std::string_view function_name,
                                    std::int64_t return_imm) {
  const std::string symbol = asm_symbol_name(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_asm(std::string_view target_triple,
                                    std::int64_t return_imm) {
  return emit_minimal_return_asm(target_triple, "main", return_imm);
}

std::string emit_minimal_return_asm(const c4c::backend::BackendModule& module,
                                    std::int64_t return_imm) {
  const std::string symbol = minimal_single_function_symbol(module);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_return_asm(const c4c::backend::bir::Module& module,
                                    std::int64_t return_imm) {
  const std::string symbol = minimal_single_function_symbol(module);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << symbol << "\n";
  out << symbol << ":\n";
  out << "  mov eax, " << return_imm << "\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_affine_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalAffineReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, symbol, true);
  const bool is_i686 = module.target_triple.find("i686") != std::string::npos;
  auto emit_param_move = [&](int index) {
    if (is_i686) {
      out << "  mov eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  mov eax, edi\n";
    } else {
      out << "  mov eax, esi\n";
    }
  };
  auto emit_param_add = [&](int index) {
    if (is_i686) {
      out << "  add eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  add eax, edi\n";
    } else {
      out << "  add eax, esi\n";
    }
  };

  if (slice.uses_first_param) {
    emit_param_move(0);
    if (slice.uses_second_param) {
      emit_param_add(1);
    }
  } else if (slice.uses_second_param) {
    emit_param_move(1);
  } else {
    out << "  mov eax, " << slice.constant << "\n";
    out << "  ret\n";
    return out.str();
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_affine_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalAffineReturnSlice& slice) {
  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, symbol, true);
  const bool is_i686 = module.target_triple.find("i686") != std::string::npos;
  auto emit_param_move = [&](int index) {
    if (is_i686) {
      out << "  mov eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  mov eax, edi\n";
    } else {
      out << "  mov eax, esi\n";
    }
  };
  auto emit_param_add = [&](int index) {
    if (is_i686) {
      out << "  add eax, DWORD PTR [esp + " << (4 * (index + 1)) << "]\n";
    } else if (index == 0) {
      out << "  add eax, edi\n";
    } else {
      out << "  add eax, esi\n";
    }
  };

  if (slice.uses_first_param) {
    emit_param_move(0);
    if (slice.uses_second_param) {
      emit_param_add(1);
    }
  } else if (slice.uses_second_param) {
    emit_param_move(1);
  } else {
    out << "  mov eax, " << slice.constant << "\n";
    out << "  ret\n";
    return out.str();
  }

  if (slice.constant > 0) {
    out << "  add eax, " << slice.constant << "\n";
  } else if (slice.constant < 0) {
    out << "  sub eax, " << -slice.constant << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_cast_return_asm(
    const c4c::backend::bir::Module& module,
    const MinimalCastReturnSlice& slice) {
  if (module.target_triple.find("i686") != std::string::npos) {
    throw_unsupported_direct_bir_module();
  }

  const std::string symbol = asm_symbol_name(module.target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, symbol, true);

  if (slice.opcode == c4c::backend::bir::CastOpcode::SExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I32 &&
      slice.result_type == c4c::backend::bir::TypeKind::I64) {
    out << "  movsxd rax, edi\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      slice.operand_type == c4c::backend::bir::TypeKind::I8 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  movzx eax, dil\n";
    out << "  ret\n";
    return out.str();
  }
  if (slice.opcode == c4c::backend::bir::CastOpcode::Trunc &&
      slice.operand_type == c4c::backend::bir::TypeKind::I64 &&
      slice.result_type == c4c::backend::bir::TypeKind::I32) {
    out << "  mov eax, edi\n";
    out << "  ret\n";
    return out.str();
  }

  throw_unsupported_direct_bir_module();
}

std::string emit_minimal_direct_call_asm(const c4c::backend::BackendModule& module,
                                         const c4c::backend::ParsedBackendMinimalDirectCallModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured zero-argument direct-call slice without helper metadata");
  }

  if (slice.return_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.return_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "helper return immediates outside the minimal mov-supported range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.helper->signature.name);
  const std::string main_symbol =
      asm_symbol_name(module,
                      slice.main_function == nullptr ? "main" : slice.main_function->signature.name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, " << slice.return_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_asm(
    std::string_view target_triple,
    const c4c::backend::ParsedBackendMinimalDirectCallLirModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "direct zero-argument LIR call slice without helper metadata");
  }

  if (slice.return_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.return_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "direct zero-argument LIR helper return immediates outside the minimal mov-supported range");
  }

  const std::string helper_symbol = asm_symbol_name(target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, false);
  out << "  mov eax, " << slice.return_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
  out << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalDirectCallAddImmModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured single-argument direct-call slice without helper metadata");
  }

  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "single-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.helper->signature.name);
  const std::string main_symbol =
      asm_symbol_name(module,
                      slice.main_function == nullptr ? "main" : slice.main_function->signature.name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_add_imm_asm(
    std::string_view target_triple,
    const c4c::backend::ParsedBackendMinimalDirectCallAddImmLirModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "direct single-argument LIR call slice without helper metadata");
  }

  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.add_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.add_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "single-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(
      target_triple, slice.main_function == nullptr ? "main" : slice.main_function->name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_identity_arg_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalDirectCallIdentityArgModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured identity direct-call slice without helper metadata");
  }

  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "identity direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(module, slice.helper->signature.name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_direct_call_identity_arg_asm(
    std::string_view target_triple,
    const c4c::backend::ParsedBackendMinimalDirectCallIdentityArgLirModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "direct identity LIR call slice without helper metadata");
  }

  if (slice.call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "identity direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(target_triple, slice.helper->name);
  const std::string main_symbol = asm_symbol_name(
      target_triple, slice.main_function == nullptr ? "main" : slice.main_function->name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
  out << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_dual_identity_direct_call_sub_asm(
    std::string_view target_triple,
    const MinimalDualIdentityDirectCallSubSlice& slice) {
  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "dual identity direct-call subtraction immediates outside the minimal x86 slice range");
  }

  const std::string lhs_symbol = asm_symbol_name(target_triple, slice.lhs_helper_name);
  const std::string rhs_symbol = asm_symbol_name(target_triple, slice.rhs_helper_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.caller_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, lhs_symbol, false);
  out << "  mov eax, edi\n"
      << "  ret\n";
  emit_function_prelude(out, rhs_symbol, false);
  out << "  mov eax, edi\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
  out << "  push rbx\n"
      << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  call " << lhs_symbol << "\n"
      << "  mov ebx, eax\n"
      << "  mov edi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << rhs_symbol << "\n"
      << "  sub ebx, eax\n"
      << "  mov eax, ebx\n"
      << "  pop rbx\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    std::string_view target_triple,
    const MinimalTwoArgDirectCallSlice& slice) {
  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "two-argument direct-call immediates outside the minimal x86 slice range");
  }

  const std::string helper_symbol = asm_symbol_name(target_triple, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.caller_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, esi\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
  out << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const MinimalTwoArgDirectCallSlice& slice) {
  return emit_minimal_two_arg_direct_call_asm(module.target_triple, slice);
}

std::string emit_minimal_member_array_runtime_asm(
    std::string_view target_triple,
    const MinimalMemberArrayRuntimeSlice& slice) {
  auto ensure_i32_imm = [](std::int64_t value, std::string_view label) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) {
      throw c4c::backend::LirAdapterError(
          c4c::backend::LirAdapterErrorKind::Unsupported,
          std::string(label) + " immediate outside the minimal x86 member-array slice range");
    }
  };
  ensure_i32_imm(slice.first_imm, "first member-array");
  ensure_i32_imm(slice.second_imm, "second member-array");

  const std::string helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const std::string main_symbol = asm_symbol_name(target_triple, slice.caller_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, true);
  if (slice.kind == MinimalMemberArrayRuntimeSlice::Kind::ByValueParam) {
    out << "  mov eax, esi\n"
        << "  ret\n";
  } else {
    out << "  mov rax, qword ptr [rdi]\n"
        << "  mov eax, dword ptr [rax + 4]\n"
        << "  ret\n";
  }
  emit_function_prelude(out, main_symbol, true);
  if (slice.kind == MinimalMemberArrayRuntimeSlice::Kind::ByValueParam) {
    out << "  sub rsp, 8\n"
        << "  mov edi, " << slice.first_imm << "\n"
        << "  mov esi, " << slice.second_imm << "\n"
        << "  call " << helper_symbol << "\n"
        << "  add rsp, 8\n"
        << "  ret\n";
  } else {
    out << "  sub rsp, 24\n"
        << "  mov dword ptr [rsp + 16], " << slice.first_imm << "\n"
        << "  mov dword ptr [rsp + 20], " << slice.second_imm << "\n"
        << "  lea rax, [rsp + 16]\n"
        << "  mov qword ptr [rsp + 8], rax\n"
        << "  lea rdi, [rsp + 8]\n"
        << "  call " << helper_symbol << "\n"
        << "  add rsp, 24\n"
        << "  ret\n";
  }
  return out.str();
}

std::string emit_minimal_two_arg_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalTwoArgDirectCallModuleView& slice) {
  if (slice.helper == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "structured two-argument direct-call slice without helper metadata");
  }

  if (slice.lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "two-argument direct-call immediates outside the minimal x86 slice range");
  }

  return emit_minimal_two_arg_direct_call_asm(
      module,
      MinimalTwoArgDirectCallSlice{
          slice.helper->signature.name,
          slice.main_function == nullptr ? "main" : slice.main_function->signature.name,
          slice.lhs_call_arg_imm,
          slice.rhs_call_arg_imm,
      });
}

std::string emit_minimal_conditional_return_asm(
    std::string_view target_triple,
    const MinimalConditionalReturnSlice& slice) {
  auto emit_value_to_eax = [](std::ostringstream& out,
                              const MinimalConditionalReturnSlice::ValueSource& value) {
    switch (value.kind) {
      case MinimalConditionalReturnSlice::ValueKind::Immediate:
        out << "  mov eax, " << value.imm << "\n";
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param0:
        out << "  mov eax, edi\n";
        return;
      case MinimalConditionalReturnSlice::ValueKind::Param1:
        out << "  mov eax, esi\n";
        return;
    }
  };
  auto cmp_rhs_operand = [](const MinimalConditionalReturnSlice::ValueSource& value) {
    switch (value.kind) {
      case MinimalConditionalReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalReturnSlice::ValueKind::Param0:
        return std::string("edi");
      case MinimalConditionalReturnSlice::ValueKind::Param1:
        return std::string("esi");
    }
    return std::string("0");
  };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
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
  emit_value_to_eax(out, slice.lhs);
  out << "  cmp eax, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_to_eax(out, slice.true_value);
  out << "  ret\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_to_eax(out, slice.false_value);
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalReturnSlice& slice) {
  return emit_minimal_conditional_return_asm(module.target_triple, slice);
}

std::string emit_minimal_conditional_affine_i8_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI8ReturnSlice& slice) {
  auto emit_compare_source_to_al = [](std::ostringstream& out,
                                      const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
        out << "  mov al, " << value.imm << "\n";
        return;
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
        out << "  mov al, dil\n";
        return;
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
        out << "  mov al, sil\n";
        return;
    }
  };
  auto cmp_rhs_operand = [](const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
        return std::string("dil");
      case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
        return std::string("sil");
    }
    return std::string("0");
  };
  auto emit_value_expr_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI8ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param0:
            out << "  movzx eax, dil\n";
            break;
          case MinimalConditionalAffineI8ReturnSlice::ValueKind::Param1:
            out << "  movzx eax, sil\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " eax, " << step.imm << "\n";
        }
      };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
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
          "conditional-i8-affine-return predicates outside the current compare-and-branch x86 slice");
  }

  emit_compare_source_to_al(out, slice.lhs);
  out << "  cmp al, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_eax(out, slice.true_value);
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_eax(out, slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i8_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalAffineI8ReturnSlice& slice) {
  return emit_minimal_conditional_affine_i8_return_asm(module.target_triple, slice);
}

std::string emit_minimal_conditional_affine_i32_return_asm(
    std::string_view target_triple,
    const MinimalConditionalAffineI32ReturnSlice& slice) {
  auto emit_compare_source_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            return;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
            out << "  mov eax, edi\n";
            return;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
            out << "  mov eax, esi\n";
            return;
        }
      };
  auto cmp_rhs_operand = [](const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
    switch (value.kind) {
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
        return std::to_string(value.imm);
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
        return std::string("edi");
      case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
        return std::string("esi");
    }
    return std::string("0");
  };
  auto emit_value_expr_to_eax =
      [](std::ostringstream& out, const MinimalConditionalAffineI32ReturnSlice::ValueExpr& value) {
        switch (value.kind) {
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Immediate:
            out << "  mov eax, " << value.imm << "\n";
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param0:
            out << "  mov eax, edi\n";
            break;
          case MinimalConditionalAffineI32ReturnSlice::ValueKind::Param1:
            out << "  mov eax, esi\n";
            break;
        }
        for (const auto& step : value.steps) {
          out << "  "
              << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
              << " eax, " << step.imm << "\n";
        }
      };

  const std::string symbol = asm_symbol_name(target_triple, slice.function_name);
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
          "conditional-i32-affine-return predicates outside the current compare-and-branch x86 slice");
  }

  emit_compare_source_to_eax(out, slice.lhs);
  out << "  cmp eax, " << cmp_rhs_operand(slice.rhs) << "\n";
  out << "  " << fail_branch << " .L" << slice.false_label << "\n";
  out << ".L" << slice.true_label << ":\n";
  emit_value_expr_to_eax(out, slice.true_value);
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  emit_value_expr_to_eax(out, slice.false_value);
  out << ".L" << slice.join_label << ":\n";
  for (const auto& step : slice.post_steps) {
    out << "  " << (step.opcode == c4c::backend::bir::BinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_affine_i32_return_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalAffineI32ReturnSlice& slice) {
  return emit_minimal_conditional_affine_i32_return_asm(module.target_triple, slice);
}

std::string emit_minimal_countdown_loop_asm(std::string_view target_triple,
                                            const MinimalCountdownLoopSlice& slice) {
  const std::string symbol = asm_symbol_name(target_triple, "main");
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

std::string emit_minimal_countdown_loop_asm(const c4c::backend::BackendModule& module,
                                            const MinimalCountdownLoopSlice& slice) {
  return emit_minimal_countdown_loop_asm(module.target_triple, slice);
}

std::string emit_minimal_conditional_phi_join_asm(
    std::string_view target_triple,
    const MinimalConditionalPhiJoinSlice& slice) {
  const std::string symbol = asm_symbol_name(target_triple, "main");
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
  for (const auto& step : slice.true_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << "  jmp .L" << slice.join_label << "\n";
  out << ".L" << slice.false_label << ":\n";
  out << "  mov eax, " << slice.false_value.base_imm << "\n";
  for (const auto& step : slice.false_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " eax, " << step.imm << "\n";
  }
  out << ".L" << slice.join_label << ":\n";
  if (slice.join_add_imm.has_value()) {
    out << "  add eax, " << *slice.join_add_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_phi_join_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalPhiJoinSlice& slice) {
  return emit_minimal_conditional_phi_join_asm(module.target_triple, slice);
}

std::string emit_minimal_local_array_asm(const c4c::backend::BackendModule& module,
                                         const MinimalLocalArraySlice& slice) {
  return emit_minimal_local_array_asm(module.target_triple, slice);
}

std::string emit_minimal_local_array_asm(std::string_view target_triple,
                                         const MinimalLocalArraySlice& slice) {
  if (slice.store0_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store0_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store1_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store1_imm > std::numeric_limits<std::int32_t>::max()) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "local-array store immediates outside the minimal mov-supported range");
  }

  const std::string symbol = asm_symbol_name(target_triple, "main");
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
  return emit_minimal_extern_global_array_load_asm(module.target_triple, slice);
}

std::string emit_minimal_extern_global_array_load_asm(
    std::string_view target_triple,
    const MinimalExternGlobalArrayLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

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

std::string emit_minimal_extern_scalar_global_load_asm(
    const c4c::backend::BackendModule& module,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  return emit_minimal_extern_scalar_global_load_asm(module.target_triple, slice);
}

std::string emit_minimal_extern_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_decl_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::ParsedBackendMinimalDeclaredDirectCallModuleView& slice) {
  static constexpr const char* kArgIntRegs[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  static constexpr const char* kArgPtrRegs[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  const std::string helper_symbol = asm_symbol_name(module, slice.parsed_call.symbol_name);
  const std::string main_symbol = asm_symbol_name(
      module, slice.main_function == nullptr ? "main" : slice.main_function->signature.name);

  if (slice.args.size() > 6) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "extern declaration call argument count exceeds minimal x86 register budget");
  }

  std::vector<std::string> emitted_string_constant_names;
  for (const auto& arg : slice.args) {
    if (arg.kind != MinimalExternCallArgSlice::Kind::Ptr || arg.operand.empty() ||
        arg.operand.front() != '@') {
      continue;
    }
    const auto* string_constant = find_string_constant(module, arg.operand.substr(1));
    if (string_constant == nullptr) {
      continue;
    }
    const auto& name = string_constant->name;
    bool duplicate = false;
    for (const auto& emitted : emitted_string_constant_names) {
      if (emitted == name) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      emitted_string_constant_names.push_back(name);
    }
  }

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (!emitted_string_constant_names.empty()) {
    out << ".section .rodata\n";
    for (const auto& name : emitted_string_constant_names) {
      const auto* string_constant = find_string_constant(module, name);
      if (string_constant == nullptr) {
        continue;
      }
      const std::string label = asm_private_data_label(module, "@" + name);
      out << label << ":\n"
          << "  .asciz \"" << escape_asm_string(string_constant->raw_bytes) << "\"\n";
    }
  }
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  for (std::size_t index = 0; index < slice.args.size(); ++index) {
    const auto& arg = slice.args[index];
    switch (arg.kind) {
      case MinimalExternCallArgSlice::Kind::I32Imm:
        out << "  mov " << kArgIntRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::I64Imm:
        out << "  mov " << kArgPtrRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::Ptr: {
        const auto* string_constant = find_string_constant(module, arg.operand.substr(1));
        if (string_constant == nullptr) {
          const std::string symbol = asm_symbol_name(module, arg.operand.substr(1));
          out << "  lea " << kArgPtrRegs[index] << ", " << symbol << "[rip]\n";
          break;
        }
        const auto label = asm_private_data_label(module, arg.operand);
        out << "  lea " << kArgPtrRegs[index] << ", " << label << "[rip]\n";
      } break;
    }
  }
  if (slice.callee != nullptr && slice.callee->signature.is_vararg) {
    out << "  mov al, 0\n";
  }
  out << "  call " << helper_symbol << "\n";
  if (!slice.return_call_result) {
    out << "  mov eax, " << slice.return_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_decl_call_asm(
    const c4c::codegen::lir::LirModule& module,
    const c4c::backend::ParsedBackendMinimalDeclaredDirectCallLirModuleView& slice) {
  static constexpr const char* kArgIntRegs[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  static constexpr const char* kArgPtrRegs[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  const std::string helper_symbol = asm_symbol_name(module.target_triple, slice.parsed_call.symbol_name);
  const std::string main_symbol = asm_symbol_name(
      module.target_triple, slice.main_function == nullptr ? "main" : slice.main_function->name);

  if (slice.args.size() > 6) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "extern declaration call argument count exceeds minimal x86 register budget");
  }

  std::vector<std::string> emitted_string_constant_names;
  for (const auto& arg : slice.args) {
    if (arg.kind != MinimalExternCallArgSlice::Kind::Ptr || arg.operand.empty() ||
        arg.operand.front() != '@') {
      continue;
    }
    const auto* string_constant = find_string_constant(module, arg.operand);
    if (string_constant == nullptr) {
      continue;
    }
    bool duplicate = false;
    for (const auto& emitted : emitted_string_constant_names) {
      if (emitted == string_constant->pool_name) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      emitted_string_constant_names.push_back(string_constant->pool_name);
    }
  }

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (!emitted_string_constant_names.empty()) {
    out << ".section .rodata\n";
    for (const auto& name : emitted_string_constant_names) {
      const auto* string_constant = find_string_constant(module, name);
      if (string_constant == nullptr) {
        continue;
      }
      const std::string label = asm_private_data_label(module.target_triple, string_constant->pool_name);
      out << label << ":\n"
          << "  .asciz \"" << escape_asm_string(string_constant->raw_bytes) << "\"\n";
    }
  }
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  for (std::size_t index = 0; index < slice.args.size(); ++index) {
    const auto& arg = slice.args[index];
    switch (arg.kind) {
      case MinimalExternCallArgSlice::Kind::I32Imm:
        out << "  mov " << kArgIntRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::I64Imm:
        out << "  mov " << kArgPtrRegs[index] << ", " << arg.imm << "\n";
        break;
      case MinimalExternCallArgSlice::Kind::Ptr: {
        const auto* string_constant = find_string_constant(module, arg.operand);
        if (string_constant == nullptr) {
          const std::string symbol =
              asm_symbol_name(module.target_triple, arg.operand.substr(1));
          out << "  lea " << kArgPtrRegs[index] << ", " << symbol << "[rip]\n";
          break;
        }
        const auto label = asm_private_data_label(module.target_triple, string_constant->pool_name);
        out << "  lea " << kArgPtrRegs[index] << ", " << label << "[rip]\n";
      } break;
    }
  }
  if (slice.callee_is_vararg) {
    out << "  mov al, 0\n";
  }
  out << "  call " << helper_symbol << "\n";
  if (!slice.return_call_result) {
    out << "  mov eax, " << slice.return_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_multi_printf_vararg_asm(
    std::string_view target_triple,
    const c4c::codegen::lir::LirModule& module,
    const MinimalMultiPrintfVarargSlice& slice) {
  const auto* first = find_string_constant(module, slice.first_string_pool_name);
  const auto* second = find_string_constant(module, slice.second_string_pool_name);
  if (first == nullptr || second == nullptr) {
    throw c4c::backend::LirAdapterError(
        c4c::backend::LirAdapterErrorKind::Unsupported,
        "bounded multi-printf vararg slice lost one of its string constants");
  }

  const std::string main_symbol = asm_symbol_name(target_triple, "main");
  const std::string printf_symbol = asm_symbol_name(target_triple, "printf");
  const std::string first_label = asm_private_data_label(target_triple, first->pool_name);
  const std::string second_label = asm_private_data_label(target_triple, second->pool_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".section .rodata\n";
  out << first_label << ":\n"
      << "  .asciz \"" << escape_asm_string(first->raw_bytes) << "\"\n";
  out << second_label << ":\n"
      << "  .asciz \"" << escape_asm_string(second->raw_bytes) << "\"\n";
  out << ".text\n";
  out << ".globl " << main_symbol << "\n";
  out << main_symbol << ":\n";
  out << "  lea rdi, " << first_label << "[rip]\n";
  out << "  mov al, 0\n";
  out << "  call " << printf_symbol << "\n";
  out << "  lea rdi, " << second_label << "[rip]\n";
  out << "  mov al, 0\n";
  out << "  call " << printf_symbol << "\n";
  out << "  mov eax, 0\n";
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_char_pointer_diff_asm(
    const c4c::backend::BackendModule& module,
    const MinimalGlobalCharPointerDiffSlice& slice) {
  return emit_minimal_global_char_pointer_diff_asm(module.target_triple, slice);
}

std::string emit_minimal_global_char_pointer_diff_asm(
    std::string_view target_triple,
    const MinimalGlobalCharPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

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
  return emit_minimal_global_int_pointer_diff_asm(module.target_triple, slice);
}

std::string emit_minimal_global_int_pointer_diff_asm(
    std::string_view target_triple,
    const MinimalGlobalIntPointerDiffSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

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
  return emit_minimal_scalar_global_load_asm(module.target_triple, slice);
}

std::string emit_minimal_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalLoadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << (slice.zero_initializer ? ".bss\n" : ".data\n")
      << ".globl " << global_symbol << "\n"
      << global_symbol << ":\n";
  if (slice.zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << slice.init_imm << "\n";
  }
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
  return emit_minimal_scalar_global_store_reload_asm(module.target_triple, slice);
}

std::string emit_minimal_scalar_global_store_reload_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalStoreReloadSlice& slice) {
  const std::string global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

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
    std::string_view target_triple,
    const MinimalStringLiteralCharSlice& slice) {
  const std::string string_label = asm_private_data_label(target_triple, slice.pool_name);
  const std::string main_symbol = asm_symbol_name(target_triple, "main");

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
      c4c::backend::stack_layout::find_assigned_reg(regalloc, slice.regalloc_source_value);
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
  const auto main_symbol = asm_symbol_name(module, slice.caller_name);

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

std::string emit_minimal_call_crossing_direct_call_asm(
    std::string_view target_triple,
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
      c4c::backend::stack_layout::find_assigned_reg(regalloc, slice.regalloc_source_value);
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
  const auto helper_symbol = asm_symbol_name(target_triple, slice.callee_name);
  const auto main_symbol = asm_symbol_name(target_triple, slice.caller_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  out << ".text\n";
  emit_function_prelude(out, helper_symbol, false);
  out << "  mov eax, edi\n"
      << "  add eax, " << slice.helper_add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, main_symbol, true);
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

std::optional<std::string> try_emit_direct_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  try {
    if (const auto slice = parse_minimal_member_array_runtime_slice(module);
        slice.has_value()) {
      return emit_minimal_member_array_runtime_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_dual_identity_direct_call_sub_slice(module);
        slice.has_value()) {
      return emit_minimal_dual_identity_direct_call_sub_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_two_arg_direct_call_slice(module);
        slice.has_value()) {
      return emit_minimal_two_arg_direct_call_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_local_array_slice(module);
        slice.has_value()) {
      return emit_minimal_local_array_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_scalar_global_load_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_global_array_load_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_char_pointer_diff_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      return emit_minimal_global_int_pointer_diff_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_roundtrip_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_load_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_load_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      return emit_minimal_scalar_global_store_reload_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      return emit_minimal_string_literal_char_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_return_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(module);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(module.target_triple, *slice);
    }
    if (const auto imm = parse_minimal_countdown_do_while_return_imm(module);
        imm.has_value()) {
      return emit_minimal_return_asm(module.target_triple, *imm);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(module.target_triple, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_lir_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(module.target_triple, *slice);
    }
    if (const auto slice =
            c4c::backend::parse_backend_minimal_direct_call_add_imm_lir_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_two_arg_direct_call_slice(module);
        slice.has_value()) {
      return emit_minimal_two_arg_direct_call_asm(module.target_triple, *slice);
    }
    if (const auto slice = parse_minimal_folded_two_arg_direct_call_return_slice(module);
        slice.has_value()) {
      return emit_minimal_return_asm(module.target_triple, slice->function_name, slice->return_imm);
    }
    if (const auto slice =
            c4c::backend::parse_backend_minimal_direct_call_identity_arg_lir_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_identity_arg_asm(module.target_triple, *slice);
    }
    if (const auto slice =
            c4c::backend::parse_backend_minimal_declared_direct_call_lir_module(module);
        slice.has_value()) {
      return emit_minimal_extern_decl_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_multi_printf_vararg_slice(module);
        slice.has_value()) {
      return emit_minimal_multi_printf_vararg_asm(module.target_triple, module, *slice);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module);
        slice.has_value()) {
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          module.target_triple, synthesize_shared_x86_call_crossing_regalloc(*slice), *slice));
    }
    if (const auto imm = parse_minimal_double_indirect_local_pointer_conditional_return_imm(module);
        imm.has_value()) {
      return emit_minimal_return_asm(module.target_triple, *imm);
    }
    if (const auto imm = parse_minimal_goto_only_constant_return_imm(module);
        imm.has_value()) {
      return emit_minimal_return_asm(module.target_triple, *imm);
    }
    if (const auto imm = parse_minimal_lir_return_imm(module); imm.has_value()) {
      return emit_minimal_return_asm(module.target_triple, *imm);
    }
    if (const auto lowered_bir = c4c::backend::try_lower_to_bir(module);
        lowered_bir.has_value()) {
      try {
        return emit_module(*lowered_bir);
      } catch (const std::invalid_argument&) {
      }
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  return std::nullopt;
}

}  // namespace

std::string emit_module(const c4c::backend::BackendModule& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  std::string backend_ir_error;
  if (!c4c::backend::validate_backend_module(module, &backend_ir_error)) {
    return c4c::backend::print_backend_module(module);
  }
  try {
    if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_scalar_global_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      return emit_minimal_extern_global_array_load_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_local_array_slice(module);
        slice.has_value()) {
      return emit_minimal_local_array_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_declared_direct_call_module(module);
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
      return emit_minimal_string_literal_char_asm(module.target_triple, *slice);
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
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(module, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_direct_call_add_imm_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(module, *slice);
    }
    if (const auto slice =
            c4c::backend::parse_backend_minimal_direct_call_identity_arg_module(module);
        slice.has_value()) {
      return emit_minimal_direct_call_identity_arg_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_dual_identity_direct_call_sub_slice(module);
        slice.has_value()) {
      return emit_minimal_dual_identity_direct_call_sub_asm(module.target_triple, *slice);
    }
    if (const auto slice = c4c::backend::parse_backend_minimal_two_arg_direct_call_module(module);
        slice.has_value()) {
      return emit_minimal_two_arg_direct_call_asm(module, *slice);
    }
    if (const auto imm = parse_minimal_folded_two_arg_direct_call_return_imm(module);
        imm.has_value()) {
      return emit_minimal_return_asm(module, *imm);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module);
        slice.has_value()) {
      return remove_redundant_self_moves(emit_minimal_call_crossing_direct_call_asm(
          module, synthesize_shared_x86_call_crossing_regalloc(*slice), *slice));
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
    if (const auto slice = parse_minimal_affine_return_slice(module);
        slice.has_value()) {
      return emit_minimal_affine_return_asm(module, *slice);
    }
  } catch (const c4c::backend::LirAdapterError&) {
  }

  if (legacy_fallback != nullptr) {
    if (const auto rendered = try_emit_direct_lir_module(*legacy_fallback);
        rendered.has_value()) {
      return *rendered;
    }
    return c4c::codegen::lir::print_llvm(*legacy_fallback);
  }
  return c4c::backend::print_backend_module(module);
}

std::string emit_module(const c4c::backend::bir::Module& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  (void)legacy_fallback;
  if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
  }
  if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
    return emit_minimal_return_asm(module, *imm);
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
  throw_unsupported_direct_bir_module();
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto rendered = try_emit_direct_lir_module(module); rendered.has_value()) {
    return *rendered;
  }

  if (const auto bir_module = c4c::backend::try_lower_to_bir(module); bir_module.has_value()) {
    try {
      return emit_module(*bir_module, &module);
    } catch (const std::invalid_argument& ex) {
      if (!is_direct_bir_subset_error(ex)) {
        throw;
      }
    }
  }

  try {
    const auto adapted = c4c::backend::lower_lir_to_backend_module(module);
    return emit_module(adapted, &module);
  } catch (const c4c::backend::LirAdapterError&) {
    return c4c::codegen::lir::print_llvm(module);
  }
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(
      assembler::AssembleRequest{.asm_text = emit_module(module), .output_path = output_path});
}

}  // namespace c4c::backend::x86
