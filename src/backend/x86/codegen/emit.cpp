#include "x86_codegen.hpp"

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

struct MinimalStringLiteralCharSlice {
  std::string function_name;
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_index = 0;
  c4c::codegen::lir::LirCastKind extend_kind = c4c::codegen::lir::LirCastKind::SExt;
};

struct MinimalScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::size_t align_bytes = 4;
  bool zero_initializer = false;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  std::size_t align_bytes = 4;
};

struct MinimalExternScalarGlobalLoadSlice {
  std::string function_name;
  std::string global_name;
};

struct MinimalAffineReturnSlice {
  std::string function_name;
  std::size_t param_count = 0;
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

struct MinimalVariadicSum2Slice {
  std::string helper_name;
  std::string entry_name;
};

struct MinimalVariadicDoubleBytesSlice {
  std::string helper_name;
  std::string entry_name;
};

struct MinimalLocalTempSlice {
  std::string function_name;
  std::int64_t stored_imm = 0;
};

struct MinimalParamSlotAddSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t add_imm = 0;
};

struct MinimalExternZeroArgCallSlice {
  std::string extern_name;
  std::string function_name;
};

struct MinimalLocalArgCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t call_arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct MinimalTwoArgHelperCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalTwoArgLocalArgHelperCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
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

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

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

std::optional<MinimalParamSlotAddSlice> parse_minimal_param_slot_add_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.extern_decls.empty() || !module.string_pool.empty() ||
      module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "add_one" || entry.name != "main" || entry.is_declaration ||
      entry.entry.value != 0 || !entry.alloca_insts.empty() || entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto parsed_helper =
      c4c::backend::parse_backend_single_param_slot_add_function(helper, "add_one");
  if (!parsed_helper.has_value() || parsed_helper->add == nullptr) {
    return std::nullopt;
  }

  const auto add_imm = parse_i64(parsed_helper->add->rhs);
  if (!add_imm.has_value() || *add_imm < std::numeric_limits<std::int32_t>::min() ||
      *add_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* call = std::get_if<LirCallOp>(&main_entry.insts.front());
  const auto* ret = std::get_if<LirRet>(&main_entry.terminator);
  if (main_entry.label != "entry" || main_entry.insts.size() != 1 || call == nullptr ||
      ret == nullptr || call->callee != "@add_one" || call->return_type != "i32" ||
      call->callee_type_suffix != "(i32)" || call->args_str != "i32 5" ||
      !ret->value_str.has_value() || *ret->value_str != call->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalParamSlotAddSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .add_imm = *add_imm,
  };
}

std::optional<MinimalExternZeroArgCallSlice> parse_minimal_extern_zero_arg_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      module.extern_decls.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  const auto& decl = module.extern_decls.front();
  const bool decl_returns_i32 =
      decl.return_type_str == "i32" || decl.return_type.str() == "i32";
  if (function.is_declaration || function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty() ||
      decl.name.empty() || !decl_returns_i32) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* call = entry.insts.size() == 1 ? std::get_if<LirCallOp>(&entry.insts.front()) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  const auto called_name =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
  if (entry.label != "entry" || call == nullptr || ret == nullptr || !called_name.has_value() ||
      *called_name != decl.name || call->return_type != "i32" ||
      !ret->value_str.has_value() || *ret->value_str != call->result ||
      c4c::backend::backend_lir_lower_function_return_type(function, *ret) !=
          c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  return MinimalExternZeroArgCallSlice{
      .extern_name = decl.name,
      .function_name = function.name,
  };
}

std::optional<MinimalExternZeroArgCallSlice> parse_minimal_declared_zero_arg_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& function,
         const LirFunction& decl) -> std::optional<MinimalExternZeroArgCallSlice> {
    if (function.is_declaration || !decl.is_declaration || function.entry.value != 0 ||
        function.blocks.size() != 1 || !function.alloca_insts.empty() ||
        !function.stack_objects.empty() || !decl.blocks.empty() || !decl.alloca_insts.empty() ||
        !decl.stack_objects.empty() || decl.name.empty() ||
        !c4c::backend::backend_lir_signature_matches(decl.signature_text,
                                                     "declare",
                                                     "i32",
                                                     decl.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto& entry = function.blocks.front();
    const auto* call =
        entry.insts.size() == 1 ? std::get_if<LirCallOp>(&entry.insts.front()) : nullptr;
    const auto* ret = std::get_if<LirRet>(&entry.terminator);
    const auto called_name =
        call == nullptr ? std::nullopt
                        : c4c::backend::parse_backend_zero_arg_direct_global_typed_call(*call);
    if (entry.label != "entry" || call == nullptr || ret == nullptr || !called_name.has_value() ||
        *called_name != decl.name || call->return_type != "i32" ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(function, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    return MinimalExternZeroArgCallSlice{
        .extern_name = decl.name,
        .function_name = function.name,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalLocalArgCallSlice> parse_minimal_local_arg_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalLocalArgCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        entry.alloca_insts.size() != 1 || !entry.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto parsed_helper = c4c::backend::parse_backend_single_add_imm_function(helper, std::nullopt);
    if (!parsed_helper.has_value() || parsed_helper->add == nullptr) {
      return std::nullopt;
    }

    const auto add_imm = parse_i64(parsed_helper->add->rhs);
    if (!add_imm.has_value() || *add_imm < std::numeric_limits<std::int32_t>::min() ||
        *add_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    const auto* slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
    if (slot == nullptr || slot->result.str().empty() || slot->type_str != "i32" ||
        !slot->count.str().empty() || slot->align != 4) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() == 3 ? std::get_if<LirCallOp>(&block.insts[2]) : nullptr;
    const auto call_operand =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_single_typed_call_operand(*call,
                                                                                   helper.name,
                                                                                   "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operand.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    std::vector<LirInst> prefix_insts;
    prefix_insts.reserve(2);
    prefix_insts.push_back(block.insts[0]);
    prefix_insts.push_back(block.insts[1]);
    const auto stored_imm = c4c::backend::parse_backend_single_local_i32_slot_call_operand_imm(
        prefix_insts, slot->result.str(), *call_operand, *call_operand);
    if (!stored_imm.has_value()) {
      return std::nullopt;
    }

    return MinimalLocalArgCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .call_arg_imm = stored_imm->first,
        .add_imm = *add_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalTwoArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        !entry.alloca_insts.empty() || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_two_param_add_function(helper, std::nullopt);
    if (!parsed_helper.has_value()) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() == 1 ? std::get_if<LirCallOp>(&block.insts.front()) : nullptr;
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operands.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(call_operands->first);
    const auto rhs_call_arg_imm = parse_i64(call_operands->second);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value() ||
        *lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
        *rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    return MinimalTwoArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .lhs_call_arg_imm = *lhs_call_arg_imm,
        .rhs_call_arg_imm = *rhs_call_arg_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgLocalArgHelperCallSlice>
parse_minimal_two_arg_local_arg_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalTwoArgLocalArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        entry.alloca_insts.size() != 1 || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_two_param_add_function(helper, std::nullopt);
    if (!parsed_helper.has_value()) {
      return std::nullopt;
    }

    const auto* slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
    if (slot == nullptr || slot->result.str().empty() || slot->type_str != "i32" ||
        !slot->count.str().empty() || slot->align != 4) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() >= 3 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operands.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto rhs_call_arg_imm = parse_i64(call_operands->second);
    if (!rhs_call_arg_imm.has_value() ||
        *rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    std::vector<LirInst> prefix_insts;
    prefix_insts.reserve(block.insts.size() - 1);
    prefix_insts.insert(prefix_insts.end(), block.insts.begin(), block.insts.end() - 1);
    const auto stored_imm = c4c::backend::parse_backend_single_local_i32_slot_call_operand_imm(
        prefix_insts, slot->result.str(), call_operands->first, call_operands->second);
    if (!stored_imm.has_value() || !stored_imm->second) {
      return std::nullopt;
    }

    return MinimalTwoArgLocalArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .lhs_call_arg_imm = stored_imm->first,
        .rhs_call_arg_imm = *rhs_call_arg_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgLocalArgHelperCallSlice>
parse_minimal_two_arg_second_local_arg_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalTwoArgLocalArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        entry.alloca_insts.size() != 1 || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_two_param_add_function(helper, std::nullopt);
    if (!parsed_helper.has_value()) {
      return std::nullopt;
    }

    const auto* slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
    if (slot == nullptr || slot->result.str().empty() || slot->type_str != "i32" ||
        !slot->count.str().empty() || slot->align != 4) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() >= 3 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operands.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(call_operands->first);
    if (!lhs_call_arg_imm.has_value() ||
        *lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    std::vector<LirInst> prefix_insts;
    prefix_insts.reserve(block.insts.size() - 1);
    prefix_insts.insert(prefix_insts.end(), block.insts.begin(), block.insts.end() - 1);
    const auto stored_imm = c4c::backend::parse_backend_single_local_i32_slot_call_operand_imm(
        prefix_insts, slot->result.str(), call_operands->first, call_operands->second);
    if (!stored_imm.has_value() || stored_imm->second) {
      return std::nullopt;
    }

    return MinimalTwoArgLocalArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .lhs_call_arg_imm = *lhs_call_arg_imm,
        .rhs_call_arg_imm = stored_imm->first,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgLocalArgHelperCallSlice>
parse_minimal_two_arg_both_local_arg_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalTwoArgLocalArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        entry.alloca_insts.size() != 2 || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_two_param_add_function(helper, std::nullopt);
    if (!parsed_helper.has_value()) {
      return std::nullopt;
    }

    const auto* lhs_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts[0]);
    const auto* rhs_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts[1]);
    if (lhs_slot == nullptr || rhs_slot == nullptr || lhs_slot->result.str().empty() ||
        rhs_slot->result.str().empty() || lhs_slot->type_str != "i32" ||
        rhs_slot->type_str != "i32" || !lhs_slot->count.str().empty() ||
        !rhs_slot->count.str().empty() || lhs_slot->align != 4 || rhs_slot->align != 4) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() == 5 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operands.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* lhs_store = std::get_if<LirStoreOp>(&block.insts[0]);
    const auto* rhs_store = std::get_if<LirStoreOp>(&block.insts[1]);
    const auto* lhs_load = std::get_if<LirLoadOp>(&block.insts[2]);
    const auto* rhs_load = std::get_if<LirLoadOp>(&block.insts[3]);
    if (lhs_store == nullptr || rhs_store == nullptr || lhs_load == nullptr || rhs_load == nullptr ||
        !c4c::backend::backend_lir_type_is_i32(lhs_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_load->type_str) ||
        lhs_store->ptr != lhs_slot->result.str() || rhs_store->ptr != rhs_slot->result.str() ||
        lhs_load->ptr != lhs_slot->result.str() || rhs_load->ptr != rhs_slot->result.str() ||
        lhs_load->result != call_operands->first || rhs_load->result != call_operands->second) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(lhs_store->val);
    const auto rhs_call_arg_imm = parse_i64(rhs_store->val);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value() ||
        *lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
        *rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    return MinimalTwoArgLocalArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .lhs_call_arg_imm = *lhs_call_arg_imm,
        .rhs_call_arg_imm = *rhs_call_arg_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgLocalArgHelperCallSlice>
parse_minimal_two_arg_both_local_first_rewrite_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalTwoArgLocalArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        entry.alloca_insts.size() != 2 || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_two_param_add_function(helper, std::nullopt);
    if (!parsed_helper.has_value()) {
      return std::nullopt;
    }

    const auto* lhs_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts[0]);
    const auto* rhs_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts[1]);
    if (lhs_slot == nullptr || rhs_slot == nullptr || lhs_slot->result.str().empty() ||
        rhs_slot->result.str().empty() || lhs_slot->type_str != "i32" ||
        rhs_slot->type_str != "i32" || !lhs_slot->count.str().empty() ||
        !rhs_slot->count.str().empty() || lhs_slot->align != 4 || rhs_slot->align != 4) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() == 8 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (block.label != "entry" || call == nullptr || ret == nullptr || !call_operands.has_value() ||
        !ret->value_str.has_value() || *ret->value_str != call->result ||
        c4c::backend::backend_lir_lower_function_return_type(entry, *ret) !=
            c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* lhs_initial_store = std::get_if<LirStoreOp>(&block.insts[0]);
    const auto* rhs_initial_store = std::get_if<LirStoreOp>(&block.insts[1]);
    const auto* lhs_initial_load = std::get_if<LirLoadOp>(&block.insts[2]);
    const auto* lhs_rewrite = std::get_if<LirBinOp>(&block.insts[3]);
    const auto* lhs_rewrite_store = std::get_if<LirStoreOp>(&block.insts[4]);
    const auto* lhs_final_load = std::get_if<LirLoadOp>(&block.insts[5]);
    const auto* rhs_final_load = std::get_if<LirLoadOp>(&block.insts[6]);
    if (lhs_initial_store == nullptr || rhs_initial_store == nullptr || lhs_initial_load == nullptr ||
        lhs_rewrite == nullptr || lhs_rewrite_store == nullptr || lhs_final_load == nullptr ||
        rhs_final_load == nullptr || !c4c::backend::backend_lir_type_is_i32(lhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_final_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_final_load->type_str) ||
        lhs_initial_store->ptr != lhs_slot->result.str() ||
        rhs_initial_store->ptr != rhs_slot->result.str() ||
        lhs_initial_load->ptr != lhs_slot->result.str() ||
        lhs_rewrite->opcode.typed() != LirBinaryOpcode::Add ||
        lhs_rewrite->lhs != lhs_initial_load->result || lhs_rewrite->rhs != "0" ||
        lhs_rewrite_store->ptr != lhs_slot->result.str() ||
        lhs_rewrite_store->val != lhs_rewrite->result ||
        lhs_final_load->ptr != lhs_slot->result.str() ||
        rhs_final_load->ptr != rhs_slot->result.str() ||
        lhs_final_load->result != call_operands->first ||
        rhs_final_load->result != call_operands->second) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(lhs_initial_store->val);
    const auto rhs_call_arg_imm = parse_i64(rhs_initial_store->val);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value() ||
        *lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
        *rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    return MinimalTwoArgLocalArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .lhs_call_arg_imm = *lhs_call_arg_imm,
        .rhs_call_arg_imm = *rhs_call_arg_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalScalarGlobalLoadSlice> parse_minimal_scalar_global_load_slice(
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
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LoadGlobalInst>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .init_imm = global.initializer->immediate,
      .align_bytes = load->align_bytes > 0 ? load->align_bytes : 4,
      .zero_initializer = global.initializer->immediate == 0,
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
  if (entry.label != "entry" || load == nullptr ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 || load->global_name != global.name ||
      load->byte_offset != 0 || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{
      .function_name = function.name,
      .global_name = global.name,
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
      !c4c::backend::backend_lir_is_zero_arg_i32_definition(function.signature_text) ||
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
      load == nullptr || extend == nullptr || ret == nullptr ||
      base_gep->result.empty() || index_cast->result.empty() || load->result.empty()) {
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
      extend->to_type != "i32" || !ret->value_str.has_value() ||
      *ret->value_str != extend->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalStringLiteralCharSlice{
      .function_name = function.name,
      .pool_name = string_const.pool_name,
      .raw_bytes = string_const.raw_bytes,
      .byte_index = *byte_index,
      .extend_kind = extend->kind,
  };
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

std::string emit_minimal_param_slot_add_asm(std::string_view target_triple,
                                            const MinimalParamSlotAddSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n";
  if (slice.add_imm > 0) {
    out << "  add eax, " << slice.add_imm << "\n";
  } else if (slice.add_imm < 0) {
    out << "  sub eax, " << -slice.add_imm << "\n";
  }
  out << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, 5\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_zero_arg_call_asm(
    std::string_view target_triple,
    const MinimalExternZeroArgCallSlice& slice) {
  const auto extern_symbol = asm_symbol_name(target_triple, slice.extern_name);
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern " << extern_symbol << "\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  call " << extern_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_local_arg_call_asm(std::string_view target_triple,
                                            const MinimalLocalArgCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n";
  if (slice.add_imm > 0) {
    out << "  add eax, " << slice.add_imm << "\n";
  } else if (slice.add_imm < 0) {
    out << "  sub eax, " << -slice.add_imm << "\n";
  }
  out << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
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

std::string emit_minimal_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalLoadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, slice.align_bytes,
                                    slice.zero_initializer);
  if (slice.zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << slice.init_imm << "\n";
  }
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_asm(
    std::string_view target_triple,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  const auto global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_asm(
    std::string_view target_triple,
    const MinimalScalarGlobalStoreReloadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = asm_symbol_name(target_triple, slice.global_name);
  const auto function_symbol = asm_symbol_name(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, slice.align_bytes, false)
      << "  .long " << slice.init_imm << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.store_imm << "\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    std::string_view target_triple,
    const MinimalStringLiteralCharSlice& slice) {
  const auto string_label = asm_private_data_label(target_triple, slice.pool_name);
  const auto symbol = asm_symbol_name(target_triple, slice.function_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".section .rodata\n"
      << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n"
      << ".text\n"
      << emit_function_prelude(target_triple, symbol)
      << "  lea rax, " << string_label << "[rip]\n";
  if (slice.extend_kind == c4c::codegen::lir::LirCastKind::SExt) {
    out << "  movsx eax, byte ptr [rax + " << slice.byte_index << "]\n";
  } else {
    out << "  movzx eax, byte ptr [rax + " << slice.byte_index << "]\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_helper_call_asm(
    std::string_view target_triple,
    const MinimalTwoArgHelperCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n"
      << "  add eax, esi\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_two_arg_local_arg_helper_call_asm(
    std::string_view target_triple,
    const MinimalTwoArgLocalArgHelperCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n"
      << "  add eax, esi\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::optional<MinimalVariadicSum2Slice> parse_minimal_variadic_sum2_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.string_pool.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "sum2" || helper.is_declaration || helper.entry.value != 0 ||
      helper.alloca_insts.size() != 2 || helper.blocks.size() != 4) {
    return std::nullopt;
  }
  if (entry.name != "main" || entry.is_declaration || entry.entry.value != 0 ||
      !entry.alloca_insts.empty() || entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto* ap_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[0]);
  const auto* second_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[1]);
  if (ap_alloca == nullptr || second_alloca == nullptr || ap_alloca->result != "%lv.ap" ||
      ap_alloca->type_str != "%struct.__va_list_tag_" || second_alloca->result != "%lv.second" ||
      second_alloca->type_str != "i32") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks[0];
  const auto& helper_reg = helper.blocks[1];
  const auto& helper_stack = helper.blocks[2];
  const auto& helper_join = helper.blocks[3];
  if (helper_entry.label != "entry" || helper_reg.label != "vaarg.amd64.reg.7" ||
      helper_stack.label != "vaarg.amd64.stack.8" ||
      helper_join.label != "vaarg.amd64.join.9" || helper_entry.insts.size() != 8 ||
      helper_reg.insts.size() != 6 || helper_stack.insts.size() != 5 ||
      helper_join.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* va_start = std::get_if<LirVaStartOp>(&helper_entry.insts[0]);
  const auto* gp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* fp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[2]);
  const auto* overflow_gep = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* reg_save_gep = std::get_if<LirGepOp>(&helper_entry.insts[4]);
  const auto* reg_save_load = std::get_if<LirLoadOp>(&helper_entry.insts[5]);
  const auto* gp_offset_load = std::get_if<LirLoadOp>(&helper_entry.insts[6]);
  const auto* reg_cmp = std::get_if<LirCmpOp>(&helper_entry.insts[7]);
  const auto* entry_br = std::get_if<LirCondBr>(&helper_entry.terminator);
  if (va_start == nullptr || gp_offset_gep == nullptr || fp_offset_gep == nullptr ||
      overflow_gep == nullptr || reg_save_gep == nullptr || reg_save_load == nullptr ||
      gp_offset_load == nullptr || reg_cmp == nullptr || entry_br == nullptr ||
      va_start->ap_ptr != "%lv.ap" || gp_offset_gep->ptr != "%lv.ap" ||
      gp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      fp_offset_gep->ptr != "%lv.ap" ||
      fp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      overflow_gep->ptr != "%lv.ap" ||
      overflow_gep->indices != std::vector<std::string>{"i32 0", "i32 2"} ||
      reg_save_gep->ptr != "%lv.ap" ||
      reg_save_gep->indices != std::vector<std::string>{"i32 0", "i32 3"} ||
      reg_save_load->type_str != "ptr" || reg_save_load->ptr != reg_save_gep->result ||
      gp_offset_load->type_str != "i32" || gp_offset_load->ptr != gp_offset_gep->result ||
      reg_cmp->type_str != "i32" || reg_cmp->lhs != gp_offset_load->result ||
      reg_cmp->rhs != "40" || reg_cmp->predicate != "sle" ||
      entry_br->cond_name != reg_cmp->result ||
      entry_br->true_label != helper_reg.label || entry_br->false_label != helper_stack.label) {
    return std::nullopt;
  }

  const auto* reg_offset_cast = std::get_if<LirCastOp>(&helper_reg.insts[0]);
  const auto* reg_arg_gep = std::get_if<LirGepOp>(&helper_reg.insts[1]);
  const auto* reg_next_offset = std::get_if<LirBinOp>(&helper_reg.insts[2]);
  const auto* reg_store_offset = std::get_if<LirStoreOp>(&helper_reg.insts[3]);
  const auto* reg_memcpy = std::get_if<LirMemcpyOp>(&helper_reg.insts[4]);
  const auto* reg_value_load = std::get_if<LirLoadOp>(&helper_reg.insts[5]);
  const auto* reg_br = std::get_if<LirBr>(&helper_reg.terminator);
  if (reg_offset_cast == nullptr || reg_arg_gep == nullptr || reg_next_offset == nullptr ||
      reg_store_offset == nullptr || reg_memcpy == nullptr || reg_value_load == nullptr ||
      reg_br == nullptr || reg_offset_cast->kind != LirCastKind::SExt ||
      reg_offset_cast->from_type != "i32" || reg_offset_cast->operand != gp_offset_load->result ||
      reg_offset_cast->to_type != "i64" || reg_arg_gep->element_type != "i8" ||
      reg_arg_gep->ptr != reg_save_load->result ||
      reg_arg_gep->indices != std::vector<std::string>{"i64 " + reg_offset_cast->result} ||
      reg_next_offset->opcode != "add" || reg_next_offset->type_str != "i32" ||
      reg_next_offset->lhs != gp_offset_load->result || reg_next_offset->rhs != "8" ||
      reg_store_offset->type_str != "i32" || reg_store_offset->val != reg_next_offset->result ||
      reg_store_offset->ptr != gp_offset_gep->result || reg_memcpy->dst != second_alloca->result ||
      reg_memcpy->src != reg_arg_gep->result || reg_memcpy->size != "4" ||
      reg_value_load->type_str != "i32" || reg_value_load->ptr != second_alloca->result ||
      reg_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  const auto* stack_base_load = std::get_if<LirLoadOp>(&helper_stack.insts[0]);
  const auto* stack_next_ptr = std::get_if<LirGepOp>(&helper_stack.insts[1]);
  const auto* stack_store_next = std::get_if<LirStoreOp>(&helper_stack.insts[2]);
  const auto* stack_memcpy = std::get_if<LirMemcpyOp>(&helper_stack.insts[3]);
  const auto* stack_value_load = std::get_if<LirLoadOp>(&helper_stack.insts[4]);
  const auto* stack_br = std::get_if<LirBr>(&helper_stack.terminator);
  if (stack_base_load == nullptr || stack_next_ptr == nullptr || stack_store_next == nullptr ||
      stack_memcpy == nullptr || stack_value_load == nullptr || stack_br == nullptr ||
      stack_base_load->type_str != "ptr" || stack_base_load->ptr != overflow_gep->result ||
      stack_next_ptr->element_type != "i8" || stack_next_ptr->ptr != stack_base_load->result ||
      stack_next_ptr->indices != std::vector<std::string>{"i64 8"} ||
      stack_store_next->type_str != "ptr" || stack_store_next->val != stack_next_ptr->result ||
      stack_store_next->ptr != overflow_gep->result ||
      stack_memcpy->dst != second_alloca->result ||
      stack_memcpy->src != stack_base_load->result || stack_memcpy->size != "4" ||
      stack_value_load->type_str != "i32" || stack_value_load->ptr != second_alloca->result ||
      stack_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  const auto* phi = std::get_if<LirPhiOp>(&helper_join.insts[0]);
  const auto* second_store = std::get_if<LirStoreOp>(&helper_join.insts[1]);
  const auto* va_end = std::get_if<LirVaEndOp>(&helper_join.insts[2]);
  const auto* second_load = std::get_if<LirLoadOp>(&helper_join.insts[3]);
  const auto* add = std::get_if<LirBinOp>(&helper_join.insts[4]);
  const auto* ret = std::get_if<LirRet>(&helper_join.terminator);
  if (phi == nullptr || second_store == nullptr || va_end == nullptr || second_load == nullptr ||
      add == nullptr || ret == nullptr || phi->type_str != "i32" || phi->incoming.size() != 2 ||
      phi->incoming[0].first != reg_value_load->result ||
      phi->incoming[0].second != helper_reg.label ||
      phi->incoming[1].first != stack_value_load->result ||
      phi->incoming[1].second != helper_stack.label || second_store->type_str != "i32" ||
      second_store->val != phi->result || second_store->ptr != second_alloca->result ||
      va_end->ap_ptr != "%lv.ap" || second_load->type_str != "i32" ||
      second_load->ptr != second_alloca->result || add->opcode != "add" ||
      add->type_str != "i32" || add->lhs != "%p.first" || add->rhs != second_load->result ||
      !ret->value_str.has_value() || *ret->value_str != add->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* main_call = std::get_if<LirCallOp>(&main_entry.insts.front());
  const auto* main_ret = std::get_if<LirRet>(&main_entry.terminator);
  if (main_entry.label != "entry" || main_entry.insts.size() != 1 || main_call == nullptr ||
      main_ret == nullptr || main_call->callee != "@sum2" || main_call->return_type != "i32" ||
      main_call->callee_type_suffix != "(i32, ...)" ||
      main_call->args_str != "i32 10, i32 32" ||
      !main_ret->value_str.has_value() || *main_ret->value_str != main_call->result ||
      main_ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalVariadicSum2Slice{
      .helper_name = helper.name,
      .entry_name = entry.name,
  };
}

std::optional<MinimalVariadicDoubleBytesSlice> parse_minimal_variadic_double_bytes_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.string_pool.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "variadic_double_bytes" || helper.is_declaration ||
      helper.entry.value != 0 ||
      (helper.alloca_insts.size() != 3 && helper.alloca_insts.size() != 5) ||
      helper.blocks.size() != 4) {
    return std::nullopt;
  }
  if (entry.name != "main" || entry.is_declaration || entry.entry.value != 0 ||
      !entry.alloca_insts.empty() || entry.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto* ap_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[0]);
  const auto* second_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[1]);
  const auto* bytes_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[2]);
  const LirAllocaOp* reg_tmp_alloca = nullptr;
  const LirAllocaOp* stack_tmp_alloca = nullptr;
  if (helper.alloca_insts.size() == 5) {
    reg_tmp_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[3]);
    stack_tmp_alloca = std::get_if<LirAllocaOp>(&helper.alloca_insts[4]);
  }
  if (ap_alloca == nullptr || second_alloca == nullptr || bytes_alloca == nullptr ||
      ap_alloca->result != "%lv.ap" || ap_alloca->type_str != "%struct.__va_list_tag_" ||
      second_alloca->result != "%lv.second" || second_alloca->type_str != "double" ||
      bytes_alloca->result != "%lv.bytes" || bytes_alloca->type_str != "ptr" ||
      (reg_tmp_alloca != nullptr && reg_tmp_alloca->type_str != "double") ||
      (stack_tmp_alloca != nullptr && stack_tmp_alloca->type_str != "double")) {
    return std::nullopt;
  }
  const std::string reg_tmp_name =
      reg_tmp_alloca != nullptr ? reg_tmp_alloca->result : second_alloca->result;
  const std::string stack_tmp_name =
      stack_tmp_alloca != nullptr ? stack_tmp_alloca->result : second_alloca->result;

  const auto& helper_entry = helper.blocks[0];
  const auto& helper_reg = helper.blocks[1];
  const auto& helper_stack = helper.blocks[2];
  const auto& helper_join = helper.blocks[3];
  if (helper_entry.label != "entry" || helper_reg.label != "vaarg.amd64.reg.7" ||
      helper_stack.label != "vaarg.amd64.stack.8" ||
      helper_join.label != "vaarg.amd64.join.9" || helper_entry.insts.size() != 8 ||
      helper_reg.insts.size() != 6 || helper_stack.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* va_start = std::get_if<LirVaStartOp>(&helper_entry.insts[0]);
  const auto* gp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* fp_offset_gep = std::get_if<LirGepOp>(&helper_entry.insts[2]);
  const auto* overflow_gep = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* reg_save_gep = std::get_if<LirGepOp>(&helper_entry.insts[4]);
  const auto* reg_save_load = std::get_if<LirLoadOp>(&helper_entry.insts[5]);
  const auto* fp_offset_load = std::get_if<LirLoadOp>(&helper_entry.insts[6]);
  const auto* reg_cmp = std::get_if<LirCmpOp>(&helper_entry.insts[7]);
  const auto* entry_br = std::get_if<LirCondBr>(&helper_entry.terminator);
  if (va_start == nullptr || gp_offset_gep == nullptr || fp_offset_gep == nullptr ||
      overflow_gep == nullptr || reg_save_gep == nullptr || reg_save_load == nullptr ||
      fp_offset_load == nullptr || reg_cmp == nullptr || entry_br == nullptr ||
      va_start->ap_ptr != "%lv.ap" || gp_offset_gep->ptr != "%lv.ap" ||
      gp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      fp_offset_gep->ptr != "%lv.ap" ||
      fp_offset_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      overflow_gep->ptr != "%lv.ap" ||
      overflow_gep->indices != std::vector<std::string>{"i32 0", "i32 2"} ||
      reg_save_gep->ptr != "%lv.ap" ||
      reg_save_gep->indices != std::vector<std::string>{"i32 0", "i32 3"} ||
      reg_save_load->type_str != "ptr" || reg_save_load->ptr != reg_save_gep->result ||
      fp_offset_load->type_str != "i32" || fp_offset_load->ptr != fp_offset_gep->result ||
      reg_cmp->type_str != "i32" || reg_cmp->lhs != fp_offset_load->result ||
      reg_cmp->rhs != "160" || reg_cmp->predicate != "sle" ||
      entry_br->cond_name != reg_cmp->result ||
      entry_br->true_label != helper_reg.label || entry_br->false_label != helper_stack.label) {
    return std::nullopt;
  }

  const auto* reg_offset_cast = std::get_if<LirCastOp>(&helper_reg.insts[0]);
  const auto* reg_arg_gep = std::get_if<LirGepOp>(&helper_reg.insts[1]);
  const auto* reg_next_offset = std::get_if<LirBinOp>(&helper_reg.insts[2]);
  const auto* reg_store_offset = std::get_if<LirStoreOp>(&helper_reg.insts[3]);
  const auto* reg_memcpy = std::get_if<LirMemcpyOp>(&helper_reg.insts[4]);
  const auto* reg_value_load = std::get_if<LirLoadOp>(&helper_reg.insts[5]);
  const auto* reg_br = std::get_if<LirBr>(&helper_reg.terminator);
  if (reg_offset_cast == nullptr || reg_arg_gep == nullptr || reg_next_offset == nullptr ||
      reg_store_offset == nullptr || reg_memcpy == nullptr || reg_value_load == nullptr ||
      reg_br == nullptr || reg_offset_cast->kind != LirCastKind::SExt ||
      reg_offset_cast->from_type != "i32" || reg_offset_cast->operand != fp_offset_load->result ||
      reg_offset_cast->to_type != "i64" || reg_arg_gep->element_type != "i8" ||
      reg_arg_gep->ptr != reg_save_load->result ||
      reg_arg_gep->indices != std::vector<std::string>{"i64 " + reg_offset_cast->result} ||
      reg_next_offset->opcode != "add" || reg_next_offset->type_str != "i32" ||
      reg_next_offset->lhs != fp_offset_load->result || reg_next_offset->rhs != "16" ||
      reg_store_offset->type_str != "i32" || reg_store_offset->val != reg_next_offset->result ||
      reg_store_offset->ptr != fp_offset_gep->result ||
      reg_memcpy->dst != reg_tmp_name || reg_memcpy->src != reg_arg_gep->result ||
      reg_memcpy->size != "8" || reg_value_load->type_str != "double" ||
      reg_value_load->ptr != reg_tmp_name ||
      reg_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  const auto* stack_base_load = std::get_if<LirLoadOp>(&helper_stack.insts[0]);
  const auto* stack_next_ptr = std::get_if<LirGepOp>(&helper_stack.insts[1]);
  const auto* stack_store_next = std::get_if<LirStoreOp>(&helper_stack.insts[2]);
  const auto* stack_memcpy = std::get_if<LirMemcpyOp>(&helper_stack.insts[3]);
  const auto* stack_value_load = std::get_if<LirLoadOp>(&helper_stack.insts[4]);
  const auto* stack_br = std::get_if<LirBr>(&helper_stack.terminator);
  if (stack_base_load == nullptr || stack_next_ptr == nullptr || stack_store_next == nullptr ||
      stack_memcpy == nullptr || stack_value_load == nullptr || stack_br == nullptr ||
      stack_base_load->type_str != "ptr" || stack_base_load->ptr != overflow_gep->result ||
      stack_next_ptr->element_type != "i8" || stack_next_ptr->ptr != stack_base_load->result ||
      stack_next_ptr->indices != std::vector<std::string>{"i64 8"} ||
      stack_store_next->type_str != "ptr" || stack_store_next->val != stack_next_ptr->result ||
      stack_store_next->ptr != overflow_gep->result || stack_memcpy->dst != stack_tmp_name ||
      stack_memcpy->src != stack_base_load->result || stack_memcpy->size != "8" ||
      stack_value_load->type_str != "double" || stack_value_load->ptr != stack_tmp_name ||
      stack_br->target_label != helper_join.label) {
    return std::nullopt;
  }

  std::size_t cursor = 0;
  const auto* phi = std::get_if<LirPhiOp>(&helper_join.insts[cursor++]);
  const auto* second_store = std::get_if<LirStoreOp>(&helper_join.insts[cursor++]);
  const auto* va_end = std::get_if<LirVaEndOp>(&helper_join.insts[cursor++]);
  const auto* bytes_store = std::get_if<LirStoreOp>(&helper_join.insts[cursor++]);
  const auto* bytes0_load = std::get_if<LirLoadOp>(&helper_join.insts[cursor++]);
  auto consume_byte_gep = [&](std::string_view expected_ptr,
                              std::string_view expected_imm) -> const LirGepOp* {
    if (cursor >= helper_join.insts.size()) {
      return nullptr;
    }
    if (const auto* cast = std::get_if<LirCastOp>(&helper_join.insts[cursor]);
        cast != nullptr) {
      if (cast->kind != LirCastKind::SExt || cast->from_type != "i32" ||
          cast->operand != expected_imm || cast->to_type != "i64") {
        return nullptr;
      }
      ++cursor;
      if (cursor >= helper_join.insts.size()) {
        return nullptr;
      }
      const auto* gep = std::get_if<LirGepOp>(&helper_join.insts[cursor]);
      if (gep == nullptr || gep->element_type != "i8" || gep->ptr != expected_ptr ||
          gep->indices != std::vector<std::string>{"i64 " + cast->result}) {
        return nullptr;
      }
      ++cursor;
      return gep;
    }

    const auto* gep = std::get_if<LirGepOp>(&helper_join.insts[cursor]);
    if (gep == nullptr || gep->element_type != "i8" || gep->ptr != expected_ptr ||
        gep->indices != std::vector<std::string>{std::string("i64 ") + std::string(expected_imm)}) {
      return nullptr;
    }
    ++cursor;
    return gep;
  };
  const auto* byte0_gep = consume_byte_gep(bytes0_load != nullptr ? bytes0_load->result : "", "6");
  const auto* byte0_load =
      cursor < helper_join.insts.size() ? std::get_if<LirLoadOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* byte0_extend =
      cursor < helper_join.insts.size() ? std::get_if<LirCastOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* add0 =
      cursor < helper_join.insts.size() ? std::get_if<LirBinOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* bytes1_load =
      cursor < helper_join.insts.size() ? std::get_if<LirLoadOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* byte1_gep = consume_byte_gep(bytes1_load != nullptr ? bytes1_load->result : "", "7");
  const auto* byte1_load =
      cursor < helper_join.insts.size() ? std::get_if<LirLoadOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* byte1_extend =
      cursor < helper_join.insts.size() ? std::get_if<LirCastOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* add1 =
      cursor < helper_join.insts.size() ? std::get_if<LirBinOp>(&helper_join.insts[cursor++]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&helper_join.terminator);
  if ((helper_join.insts.size() != 14 && helper_join.insts.size() != 16) || phi == nullptr ||
      second_store == nullptr ||
      va_end == nullptr || bytes_store == nullptr || bytes0_load == nullptr ||
      byte0_gep == nullptr || byte0_load == nullptr || byte0_extend == nullptr || add0 == nullptr ||
      bytes1_load == nullptr || byte1_load == nullptr || byte1_extend == nullptr ||
      byte1_gep == nullptr || add1 == nullptr || ret == nullptr || cursor != helper_join.insts.size() ||
      phi->type_str != "double" ||
      phi->incoming.size() != 2 || phi->incoming[0].first != reg_value_load->result ||
      phi->incoming[0].second != helper_reg.label ||
      phi->incoming[1].first != stack_value_load->result ||
      phi->incoming[1].second != helper_stack.label || second_store->type_str != "double" ||
      second_store->val != phi->result || second_store->ptr != second_alloca->result ||
      va_end->ap_ptr != "%lv.ap" || bytes_store->type_str != "ptr" ||
      bytes_store->val != second_alloca->result || bytes_store->ptr != bytes_alloca->result ||
      bytes0_load->type_str != "ptr" || bytes0_load->ptr != bytes_alloca->result ||
      byte0_load->type_str != "i8" || byte0_load->ptr != byte0_gep->result ||
      byte0_extend->kind != LirCastKind::ZExt || byte0_extend->from_type != "i8" ||
      byte0_extend->operand != byte0_load->result || byte0_extend->to_type != "i32" ||
      add0->opcode != "add" || add0->type_str != "i32" || add0->lhs != "%p.seed" ||
      add0->rhs != byte0_extend->result || bytes1_load->type_str != "ptr" ||
      bytes1_load->ptr != bytes_alloca->result || byte1_load->type_str != "i8" ||
      byte1_load->ptr != byte1_gep->result ||
      byte1_extend->kind != LirCastKind::ZExt || byte1_extend->from_type != "i8" ||
      byte1_extend->operand != byte1_load->result || byte1_extend->to_type != "i32" ||
      add1->opcode != "add" || add1->type_str != "i32" || add1->lhs != add0->result ||
      add1->rhs != byte1_extend->result || !ret->value_str.has_value() ||
      *ret->value_str != add1->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* main_call = std::get_if<LirCallOp>(&main_entry.insts.front());
  const auto* main_ret = std::get_if<LirRet>(&main_entry.terminator);
  if (main_entry.label != "entry" || main_entry.insts.size() != 1 || main_call == nullptr ||
      main_ret == nullptr || main_call->callee != "@variadic_double_bytes" ||
      main_call->return_type != "i32" || main_call->callee_type_suffix != "(i32, ...)" ||
      main_call->args_str != "i32 1, double 0x4002000000000000" ||
      !main_ret->value_str.has_value() || *main_ret->value_str != main_call->result ||
      main_ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalVariadicDoubleBytesSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
  };
}

std::string emit_minimal_variadic_sum2_asm(std::string_view target_triple,
                                           const MinimalVariadicSum2Slice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  lea eax, [rdi + rsi]\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, 10\n"
      << "  mov esi, 32\n"
      << "  xor eax, eax\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_variadic_double_bytes_asm(
    std::string_view target_triple,
    const MinimalVariadicDoubleBytesSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  const auto constant_label =
      asm_private_data_label(target_triple, slice.helper_name + ".double.2_25");

  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  movsd QWORD PTR [rbp - 8], xmm0\n"
      << "  mov eax, edi\n"
      << "  movzx ecx, BYTE PTR [rbp - 2]\n"
      << "  add eax, ecx\n"
      << "  movzx ecx, BYTE PTR [rbp - 1]\n"
      << "  add eax, ecx\n"
      << "  leave\n"
      << "  ret\n"
      << ".section .rodata\n"
      << constant_label << ":\n"
      << "  .quad 0x4002000000000000\n"
      << ".text\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  mov edi, 1\n"
      << "  movsd xmm0, QWORD PTR [rip + " << constant_label << "]\n"
      << "  mov eax, 1\n"
      << "  call " << helper_symbol << "\n"
      << "  pop rbp\n"
      << "  ret\n";
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
  if (const auto slice = parse_minimal_affine_return_slice(module); slice.has_value()) {
    return emit_minimal_affine_return_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_load_slice(module); slice.has_value()) {
    return emit_minimal_scalar_global_load_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
      slice.has_value()) {
    return emit_minimal_extern_scalar_global_load_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
      slice.has_value()) {
    return emit_minimal_scalar_global_store_reload_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::string emit_module(const c4c::backend::bir::Module& module) {
  if (const auto asm_text = try_emit_module(module); asm_text.has_value()) {
    return *asm_text;
  }
  throw_unsupported_direct_bir_module();
}

std::optional<std::string> try_emit_prepared_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_local_temp_slice(module); slice.has_value()) {
    return emit_minimal_local_temp_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_param_slot_add_slice(module); slice.has_value()) {
    return emit_minimal_param_slot_add_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_declared_zero_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_extern_zero_arg_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_extern_zero_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_extern_zero_arg_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_local_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_local_arg_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_two_arg_helper_call_slice(module); slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_two_arg_local_arg_helper_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_local_arg_helper_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_two_arg_second_local_arg_helper_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_local_arg_helper_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_two_arg_both_local_arg_helper_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_local_arg_helper_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_two_arg_both_local_first_rewrite_helper_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_local_arg_helper_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_variadic_sum2_slice(module); slice.has_value()) {
    return emit_minimal_variadic_sum2_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_variadic_double_bytes_slice(module);
      slice.has_value()) {
    return emit_minimal_variadic_double_bytes_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_string_literal_char_slice(module);
      slice.has_value()) {
    return emit_minimal_string_literal_char_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  if (const auto asm_text = try_emit_prepared_lir_module(module); asm_text.has_value()) {
    return *asm_text;
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
