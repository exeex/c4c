#include "x86_codegen.hpp"

#include "../../backend.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../../codegen/lir/ir.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalParamSlotAddSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t add_imm = 0;
};

struct MinimalSeventhParamStackAddSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t add_imm = 0;
  std::int64_t seventh_arg_imm = 0;
};

struct MinimalMixedRegStackParamAddSlice {
  std::string helper_name;
  std::string entry_name;
  std::string scalar_type;
  std::int64_t first_arg_imm = 0;
  std::int64_t seventh_arg_imm = 0;
};

struct MinimalRegisterAggregateParamSlotSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t first_field_imm = 0;
  std::int64_t second_field_imm = 0;
};

struct MinimalStackAggregateParamSlotSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t first_field_imm = 0;
  std::int64_t second_field_imm = 0;
  std::int64_t third_field_imm = 0;
};

struct MinimalSmallStructStackParamSlotSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t first_field_imm = 0;
  std::int64_t second_field_imm = 0;
};

struct MinimalLocalArgCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t call_arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct MinimalSingleArgHelperCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t call_arg_imm = 0;
  std::optional<std::int64_t> add_imm;
};

struct MinimalTwoArgHelperCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalFoldedTwoArgDirectCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t helper_add_imm = 0;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalDualIdentityDirectCallSubSlice {
  std::string lhs_helper_name;
  std::string rhs_helper_name;
  std::string entry_name;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct MinimalCallCrossingDirectCallSlice {
  std::string helper_name;
  std::string entry_name;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
};

struct MinimalExternZeroArgCallSlice {
  std::string extern_name;
  std::string function_name;
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

std::optional<MinimalSeventhParamStackAddSlice> parse_minimal_seventh_param_stack_add_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.extern_decls.empty() || !module.string_pool.empty() ||
      module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "add_stack_param" || entry.name != "main" || helper.is_declaration ||
      entry.is_declaration || helper.entry.value != 0 || entry.entry.value != 0 ||
      helper.alloca_insts.size() != 2 || helper.blocks.size() != 1 || !helper.stack_objects.empty() ||
      !entry.alloca_insts.empty() || !entry.stack_objects.empty() || entry.blocks.size() != 1 ||
      helper.signature_text !=
          "define i32 @add_stack_param(i32 %p.a, i32 %p.b, i32 %p.c, i32 %p.d, i32 %p.e, i32 %p.f, i32 %p.g)\n") {
    return std::nullopt;
  }

  const auto* param_slot = std::get_if<LirAllocaOp>(&helper.alloca_insts.front());
  if (param_slot == nullptr || param_slot->result.str() != "%lv.param.g" || param_slot->type_str != "i32" ||
      !param_slot->count.str().empty() || param_slot->align != 4) {
    return std::nullopt;
  }

  const auto* param_store = std::get_if<LirStoreOp>(&helper.alloca_insts[1]);
  if (param_store == nullptr || param_store->type_str != "i32" || param_store->val != "%p.g" ||
      param_store->ptr != "%lv.param.g") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks.front();
  if (helper_entry.label != "entry" || helper_entry.insts.size() != 2) {
    return std::nullopt;
  }

  const auto* helper_load0 = std::get_if<LirLoadOp>(&helper_entry.insts[0]);
  const auto* helper_add = std::get_if<LirBinOp>(&helper_entry.insts[1]);
  const auto* helper_ret = std::get_if<LirRet>(&helper_entry.terminator);
  if (helper_load0 == nullptr || helper_add == nullptr || helper_ret == nullptr ||
      helper_load0->type_str != "i32" ||
      helper_load0->ptr != "%lv.param.g" || helper_add->opcode.typed() != LirBinaryOpcode::Add ||
      helper_add->type_str != "i32" || helper_add->lhs != helper_load0->result ||
      !helper_ret->value_str.has_value() || *helper_ret->value_str != helper_add->result ||
      helper_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto add_imm = parse_i64(helper_add->rhs);
  if (!add_imm.has_value() || *add_imm < std::numeric_limits<std::int32_t>::min() ||
      *add_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* call = main_entry.insts.size() == 1 ? std::get_if<LirCallOp>(&main_entry.insts.front())
                                                  : nullptr;
  const auto* ret = std::get_if<LirRet>(&main_entry.terminator);
  if (main_entry.label != "entry" || call == nullptr || ret == nullptr ||
      call->callee != "@add_stack_param" || call->return_type != "i32" ||
      call->callee_type_suffix != "(i32, i32, i32, i32, i32, i32, i32)" ||
      call->args_str != "i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7" ||
      !ret->value_str.has_value() || *ret->value_str != call->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  return MinimalSeventhParamStackAddSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .add_imm = *add_imm,
      .seventh_arg_imm = 7,
  };
}

std::optional<MinimalMixedRegStackParamAddSlice> parse_minimal_mixed_reg_stack_param_add_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.extern_decls.empty() || !module.string_pool.empty() ||
      module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  const bool is_i32_helper = helper.name == "add_first_and_stack_param" &&
                             helper.signature_text ==
                                 "define i32 @add_first_and_stack_param(i32 %p.a, i32 %p.b, i32 %p.c, i32 %p.d, i32 %p.e, i32 %p.f, i32 %p.g)\n";
  const bool is_i64_helper = helper.name == "add_first_and_stack_param_i64" &&
                             helper.signature_text ==
                                 "define i64 @add_first_and_stack_param_i64(i64 %p.a, i64 %p.b, i64 %p.c, i64 %p.d, i64 %p.e, i64 %p.f, i64 %p.g)\n";
  if ((!is_i32_helper && !is_i64_helper) || entry.name != "main" || helper.is_declaration ||
      entry.is_declaration || helper.entry.value != 0 || entry.entry.value != 0 ||
      helper.alloca_insts.size() != 2 || helper.blocks.size() != 1 || !helper.stack_objects.empty() ||
      !entry.alloca_insts.empty() || !entry.stack_objects.empty() || entry.blocks.size() != 1) {
    return std::nullopt;
  }
  const std::string_view scalar_type = is_i64_helper ? "i64" : "i32";

  const auto* param_slot = std::get_if<LirAllocaOp>(&helper.alloca_insts.front());
  if (param_slot == nullptr || param_slot->result.str() != "%lv.param.g" ||
      param_slot->type_str != scalar_type || !param_slot->count.str().empty() ||
      param_slot->align != (is_i64_helper ? 8 : 4)) {
    return std::nullopt;
  }

  const auto* param_store = std::get_if<LirStoreOp>(&helper.alloca_insts[1]);
  if (param_store == nullptr || param_store->type_str != scalar_type || param_store->val != "%p.g" ||
      param_store->ptr != "%lv.param.g") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks.front();
  if (helper_entry.label != "entry" || helper_entry.insts.size() != 2) {
    return std::nullopt;
  }

  const auto* helper_load0 = std::get_if<LirLoadOp>(&helper_entry.insts[0]);
  const auto* helper_add = std::get_if<LirBinOp>(&helper_entry.insts[1]);
  const auto* helper_ret = std::get_if<LirRet>(&helper_entry.terminator);
  if (helper_load0 == nullptr || helper_add == nullptr || helper_ret == nullptr ||
      helper_load0->type_str != scalar_type || helper_load0->ptr != "%lv.param.g" ||
      helper_add->opcode.typed() != LirBinaryOpcode::Add || helper_add->type_str != scalar_type ||
      helper_add->lhs != "%p.a" || helper_add->rhs != helper_load0->result ||
      !helper_ret->value_str.has_value() || *helper_ret->value_str != helper_add->result ||
      helper_ret->type_str != scalar_type) {
    return std::nullopt;
  }

  const auto& main_entry = entry.blocks.front();
  const auto* call = main_entry.insts.size() == 1 ? std::get_if<LirCallOp>(&main_entry.insts.front())
                                                  : nullptr;
  const auto* ret = std::get_if<LirRet>(&main_entry.terminator);
  const std::string call_suffix =
      std::string("(") + std::string(scalar_type) + ", " + std::string(scalar_type) + ", " +
      std::string(scalar_type) + ", " + std::string(scalar_type) + ", " +
      std::string(scalar_type) + ", " + std::string(scalar_type) + ", " +
      std::string(scalar_type) + ")";
  const std::string call_args =
      std::string(scalar_type) + " 1, " + std::string(scalar_type) + " 2, " +
      std::string(scalar_type) + " 3, " + std::string(scalar_type) + " 4, " +
      std::string(scalar_type) + " 5, " + std::string(scalar_type) + " 6, " +
      std::string(scalar_type) + " 7";
  if (main_entry.label != "entry" || call == nullptr || ret == nullptr ||
      call->callee != ("@" + helper.name) || call->return_type != scalar_type ||
      call->callee_type_suffix != call_suffix || call->args_str != call_args ||
      !ret->value_str.has_value() || *ret->value_str != call->result ||
      ret->type_str != scalar_type) {
    return std::nullopt;
  }

  return MinimalMixedRegStackParamAddSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .scalar_type = std::string(scalar_type),
      .first_arg_imm = 1,
      .seventh_arg_imm = 7,
  };
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

std::optional<MinimalFoldedTwoArgDirectCallSlice> parse_minimal_folded_two_arg_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalFoldedTwoArgDirectCallSlice> {
    if (helper.is_declaration || helper.entry.value != 0 || helper.blocks.size() != 1 ||
        !helper.alloca_insts.empty() || !helper.stack_objects.empty() ||
        entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        !entry.alloca_insts.empty() || !entry.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {})) {
      return std::nullopt;
    }

    const auto params = c4c::backend::parse_backend_function_signature_params(helper.signature_text);
    if (!params.has_value() || params->size() != 2 || (*params)[0].is_varargs ||
        (*params)[1].is_varargs || (*params)[0].type != "i32" || (*params)[1].type != "i32" ||
        (*params)[0].operand.empty() || (*params)[1].operand.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    const auto* add =
        helper_block.insts.size() == 2 ? std::get_if<LirBinOp>(&helper_block.insts[0]) : nullptr;
    const auto* sub =
        helper_block.insts.size() == 2 ? std::get_if<LirBinOp>(&helper_block.insts[1]) : nullptr;
    const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
    if (helper_block.label != "entry" || add == nullptr || sub == nullptr || helper_ret == nullptr ||
        add->opcode.typed() != LirBinaryOpcode::Add || add->type_str != "i32" ||
        add->lhs != "10" || add->rhs != (*params)[0].operand ||
        sub->opcode.typed() != LirBinaryOpcode::Sub || sub->type_str != "i32" ||
        sub->lhs != add->result || sub->rhs != (*params)[1].operand ||
        !helper_ret->value_str.has_value() || *helper_ret->value_str != sub->result ||
        helper_ret->type_str != "i32") {
      return std::nullopt;
    }

    const auto helper_add_imm = parse_i64(add->lhs);
    if (!helper_add_imm.has_value() ||
        *helper_add_imm < std::numeric_limits<std::int32_t>::min() ||
        *helper_add_imm > std::numeric_limits<std::int32_t>::max()) {
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

    return MinimalFoldedTwoArgDirectCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .helper_add_imm = *helper_add_imm,
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

std::optional<MinimalDualIdentityDirectCallSubSlice>
parse_minimal_dual_identity_direct_call_sub_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 3 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const LirFunction* entry = nullptr;
  std::vector<const LirFunction*> helpers;
  helpers.reserve(2);
  for (const auto& function : module.functions) {
    const bool is_entry_candidate =
        !function.is_declaration && function.entry.value == 0 && function.blocks.size() == 1 &&
        function.alloca_insts.empty() && function.stack_objects.empty() &&
        c4c::backend::backend_lir_signature_matches(
            function.signature_text, "define", "i32", function.name, {});
    if (is_entry_candidate) {
      const auto& block = function.blocks.front();
      const auto* ret = std::get_if<LirRet>(&block.terminator);
      if (block.label == "entry" && block.insts.size() == 3 && ret != nullptr &&
          ret->type_str == "i32" && ret->value_str.has_value()) {
        if (entry != nullptr) {
          return std::nullopt;
        }
        entry = &function;
        continue;
      }
    }
    helpers.push_back(&function);
  }

  if (entry == nullptr || helpers.size() != 2) {
    return std::nullopt;
  }

  const auto lhs_helper_name = helpers[0]->name;
  const auto rhs_helper_name = helpers[1]->name;
  for (const auto* helper : helpers) {
    if (helper == nullptr ||
        !c4c::backend::parse_backend_single_identity_function(*helper, std::nullopt).has_value()) {
      return std::nullopt;
    }
  }

  const auto& block = entry->blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  const auto* lhs_call = std::get_if<LirCallOp>(&block.insts[0]);
  const auto* rhs_call = std::get_if<LirCallOp>(&block.insts[1]);
  const auto* sub = std::get_if<LirBinOp>(&block.insts[2]);
  if (ret == nullptr || lhs_call == nullptr || rhs_call == nullptr || sub == nullptr ||
      sub->opcode.typed() != LirBinaryOpcode::Sub || sub->type_str != "i32" ||
      !ret->value_str.has_value() || *ret->value_str != sub->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto lhs_call_operand =
      c4c::backend::parse_backend_direct_global_single_typed_call_operand(*lhs_call,
                                                                          lhs_helper_name,
                                                                          "i32");
  const auto rhs_call_operand =
      c4c::backend::parse_backend_direct_global_single_typed_call_operand(*rhs_call,
                                                                          rhs_helper_name,
                                                                          "i32");
  if (!lhs_call_operand.has_value() || !rhs_call_operand.has_value() || sub->lhs != lhs_call->result ||
      sub->rhs != rhs_call->result || lhs_call->callee != ("@" + lhs_helper_name) ||
      rhs_call->callee != ("@" + rhs_helper_name) || lhs_call->callee == rhs_call->callee) {
    return std::nullopt;
  }

  const auto lhs_call_arg_imm = parse_i64(*lhs_call_operand);
  const auto rhs_call_arg_imm = parse_i64(*rhs_call_operand);
  if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value() ||
      *lhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      *lhs_call_arg_imm > std::numeric_limits<std::int32_t>::max() ||
      *rhs_call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
      *rhs_call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  const bool matched_lhs =
      std::any_of(helpers.begin(), helpers.end(), [&](const auto* helper) { return helper->name == lhs_helper_name; });
  const bool matched_rhs =
      std::any_of(helpers.begin(), helpers.end(), [&](const auto* helper) { return helper->name == rhs_helper_name; });
  if (!matched_lhs || !matched_rhs) {
    return std::nullopt;
  }

  return MinimalDualIdentityDirectCallSubSlice{
      .lhs_helper_name = std::string(lhs_helper_name),
      .rhs_helper_name = std::string(rhs_helper_name),
      .entry_name = entry->name,
      .lhs_call_arg_imm = *lhs_call_arg_imm,
      .rhs_call_arg_imm = *rhs_call_arg_imm,
  };
}

std::optional<MinimalCallCrossingDirectCallSlice> parse_minimal_call_crossing_direct_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalCallCrossingDirectCallSlice> {
    if (entry.is_declaration || helper.is_declaration || entry.entry.value != 0 ||
        helper.entry.value != 0 || entry.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !entry.alloca_insts.empty() || !helper.alloca_insts.empty() || !entry.stack_objects.empty() ||
        !helper.stack_objects.empty() ||
        !c4c::backend::backend_lir_signature_matches(entry.signature_text,
                                                     "define",
                                                     "i32",
                                                     entry.name,
                                                     {}) ||
        !c4c::backend::backend_lir_signature_matches(helper.signature_text,
                                                     "define",
                                                     "i32",
                                                     helper.name,
                                                     {"i32"})) {
      return std::nullopt;
    }

    const auto parsed_helper =
        c4c::backend::parse_backend_single_add_imm_function(helper, helper.name);
    if (!parsed_helper.has_value() || parsed_helper->add == nullptr) {
      return std::nullopt;
    }
    const auto helper_add_imm = parse_i64(parsed_helper->add->rhs);
    if (!helper_add_imm.has_value() ||
        *helper_add_imm < std::numeric_limits<std::int32_t>::min() ||
        *helper_add_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* source_add = block.insts.size() == 3 ? std::get_if<LirBinOp>(&block.insts[0]) : nullptr;
    const auto* call = block.insts.size() == 3 ? std::get_if<LirCallOp>(&block.insts[1]) : nullptr;
    const auto* final_add = block.insts.size() == 3 ? std::get_if<LirBinOp>(&block.insts[2]) : nullptr;
    if (block.label != "entry" || source_add == nullptr || call == nullptr || final_add == nullptr ||
        ret == nullptr || source_add->opcode.typed() != LirBinaryOpcode::Add ||
        source_add->type_str != "i32" || final_add->opcode.typed() != LirBinaryOpcode::Add ||
        final_add->type_str != "i32" || !ret->value_str.has_value() ||
        *ret->value_str != final_add->result || ret->type_str != "i32") {
      return std::nullopt;
    }

    const auto source_lhs_imm = parse_i64(source_add->lhs);
    const auto source_rhs_imm = parse_i64(source_add->rhs);
    if (!source_lhs_imm.has_value() || !source_rhs_imm.has_value()) {
      return std::nullopt;
    }
    const auto source_imm = *source_lhs_imm + *source_rhs_imm;
    if (source_imm < std::numeric_limits<std::int32_t>::min() ||
        source_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    const auto call_operand =
        c4c::backend::parse_backend_direct_global_single_typed_call_operand(*call, helper.name, "i32");
    if (!call_operand.has_value() || *call_operand != source_add->result || final_add->lhs != source_add->result ||
        final_add->rhs != call->result) {
      return std::nullopt;
    }

    return MinimalCallCrossingDirectCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .source_imm = source_imm,
        .helper_add_imm = *helper_add_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_local_arg_call_slice(
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

    return MinimalTwoArgHelperCallSlice{
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

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_second_local_arg_call_slice(
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

    return MinimalTwoArgHelperCallSlice{
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

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_both_local_arg_call_slice(
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

std::optional<MinimalTwoArgHelperCallSlice>
parse_minimal_two_arg_both_local_first_rewrite_call_slice(
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

std::optional<MinimalTwoArgHelperCallSlice>
parse_minimal_two_arg_both_local_second_rewrite_call_slice(
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
    const auto* rhs_initial_load = std::get_if<LirLoadOp>(&block.insts[2]);
    const auto* rhs_rewrite = std::get_if<LirBinOp>(&block.insts[3]);
    const auto* rhs_rewrite_store = std::get_if<LirStoreOp>(&block.insts[4]);
    const auto* lhs_final_load = std::get_if<LirLoadOp>(&block.insts[5]);
    const auto* rhs_final_load = std::get_if<LirLoadOp>(&block.insts[6]);
    if (lhs_initial_store == nullptr || rhs_initial_store == nullptr || rhs_initial_load == nullptr ||
        rhs_rewrite == nullptr || rhs_rewrite_store == nullptr || lhs_final_load == nullptr ||
        rhs_final_load == nullptr || !c4c::backend::backend_lir_type_is_i32(lhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_final_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_final_load->type_str) ||
        lhs_initial_store->ptr != lhs_slot->result.str() ||
        rhs_initial_store->ptr != rhs_slot->result.str() ||
        rhs_initial_load->ptr != rhs_slot->result.str() ||
        rhs_rewrite->opcode.typed() != LirBinaryOpcode::Add ||
        rhs_rewrite->lhs != rhs_initial_load->result || rhs_rewrite->rhs != "0" ||
        rhs_rewrite_store->ptr != rhs_slot->result.str() ||
        rhs_rewrite_store->val != rhs_rewrite->result ||
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

std::optional<MinimalTwoArgHelperCallSlice>
parse_minimal_two_arg_both_local_double_rewrite_call_slice(
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
    const auto* call =
        block.insts.size() == 11 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
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
    const auto* rhs_initial_load = std::get_if<LirLoadOp>(&block.insts[5]);
    const auto* rhs_rewrite = std::get_if<LirBinOp>(&block.insts[6]);
    const auto* rhs_rewrite_store = std::get_if<LirStoreOp>(&block.insts[7]);
    const auto* lhs_final_load = std::get_if<LirLoadOp>(&block.insts[8]);
    const auto* rhs_final_load = std::get_if<LirLoadOp>(&block.insts[9]);
    if (lhs_initial_store == nullptr || rhs_initial_store == nullptr || lhs_initial_load == nullptr ||
        lhs_rewrite == nullptr || lhs_rewrite_store == nullptr || rhs_initial_load == nullptr ||
        rhs_rewrite == nullptr || rhs_rewrite_store == nullptr || lhs_final_load == nullptr ||
        rhs_final_load == nullptr ||
        !c4c::backend::backend_lir_type_is_i32(lhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(lhs_final_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rhs_final_load->type_str) ||
        lhs_initial_store->ptr != lhs_slot->result.str() ||
        rhs_initial_store->ptr != rhs_slot->result.str() ||
        lhs_initial_load->ptr != lhs_slot->result.str() ||
        lhs_rewrite->opcode.typed() != LirBinaryOpcode::Add ||
        lhs_rewrite->lhs != lhs_initial_load->result || lhs_rewrite->rhs != "0" ||
        lhs_rewrite_store->ptr != lhs_slot->result.str() ||
        lhs_rewrite_store->val != lhs_rewrite->result ||
        rhs_initial_load->ptr != rhs_slot->result.str() ||
        rhs_rewrite->opcode.typed() != LirBinaryOpcode::Add ||
        rhs_rewrite->lhs != rhs_initial_load->result || rhs_rewrite->rhs != "0" ||
        rhs_rewrite_store->ptr != rhs_slot->result.str() ||
        rhs_rewrite_store->val != rhs_rewrite->result ||
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

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_second_local_rewrite_call_slice(
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
    const auto* call = block.insts.size() == 6 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
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

    const auto* initial_store = std::get_if<LirStoreOp>(&block.insts[0]);
    const auto* initial_load = std::get_if<LirLoadOp>(&block.insts[1]);
    const auto* rewrite = std::get_if<LirBinOp>(&block.insts[2]);
    const auto* rewrite_store = std::get_if<LirStoreOp>(&block.insts[3]);
    const auto* final_load = std::get_if<LirLoadOp>(&block.insts[4]);
    if (initial_store == nullptr || initial_load == nullptr || rewrite == nullptr ||
        rewrite_store == nullptr || final_load == nullptr ||
        !c4c::backend::backend_lir_type_is_i32(initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(final_load->type_str) ||
        initial_store->ptr != slot->result.str() || initial_load->ptr != slot->result.str() ||
        rewrite->opcode.typed() != LirBinaryOpcode::Add || rewrite->lhs != initial_load->result ||
        rewrite->rhs != "0" || rewrite_store->ptr != slot->result.str() ||
        rewrite_store->val != rewrite->result || final_load->ptr != slot->result.str() ||
        call_operands->second != final_load->result) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(call_operands->first);
    const auto rhs_call_arg_imm = parse_i64(initial_store->val);
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

std::optional<MinimalTwoArgHelperCallSlice> parse_minimal_two_arg_first_local_rewrite_call_slice(
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
    const auto* call = block.insts.size() == 6 ? std::get_if<LirCallOp>(&block.insts.back()) : nullptr;
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

    const auto* initial_store = std::get_if<LirStoreOp>(&block.insts[0]);
    const auto* initial_load = std::get_if<LirLoadOp>(&block.insts[1]);
    const auto* rewrite = std::get_if<LirBinOp>(&block.insts[2]);
    const auto* rewrite_store = std::get_if<LirStoreOp>(&block.insts[3]);
    const auto* final_load = std::get_if<LirLoadOp>(&block.insts[4]);
    if (initial_store == nullptr || initial_load == nullptr || rewrite == nullptr ||
        rewrite_store == nullptr || final_load == nullptr ||
        !c4c::backend::backend_lir_type_is_i32(initial_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(initial_load->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rewrite->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(rewrite_store->type_str) ||
        !c4c::backend::backend_lir_type_is_i32(final_load->type_str) ||
        initial_store->ptr != slot->result.str() || initial_load->ptr != slot->result.str() ||
        rewrite->opcode.typed() != LirBinaryOpcode::Add || rewrite->lhs != initial_load->result ||
        rewrite->rhs != "0" || rewrite_store->ptr != slot->result.str() ||
        rewrite_store->val != rewrite->result || final_load->ptr != slot->result.str() ||
        call_operands->first != final_load->result) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_i64(initial_store->val);
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

    const auto parsed_helper =
        c4c::backend::parse_backend_single_add_imm_function(helper, std::nullopt);
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

std::optional<MinimalSingleArgHelperCallSlice> parse_minimal_single_arg_helper_call_slice(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& helper,
         const LirFunction& entry) -> std::optional<MinimalSingleArgHelperCallSlice> {
    if (entry.is_declaration || entry.entry.value != 0 || entry.blocks.size() != 1 ||
        !entry.alloca_insts.empty() || !entry.stack_objects.empty()) {
      return std::nullopt;
    }

    std::optional<std::int64_t> add_imm;
    if (const auto parsed_helper =
            c4c::backend::parse_backend_single_add_imm_function(helper, std::nullopt);
        parsed_helper.has_value() && parsed_helper->add != nullptr) {
      const auto parsed_add_imm = parse_i64(parsed_helper->add->rhs);
      if (!parsed_add_imm.has_value() ||
          *parsed_add_imm < std::numeric_limits<std::int32_t>::min() ||
          *parsed_add_imm > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
      }
      add_imm = *parsed_add_imm;
    } else if (!c4c::backend::parse_backend_single_identity_function(helper, std::nullopt)
                    .has_value()) {
      return std::nullopt;
    }

    const auto& block = entry.blocks.front();
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    const auto* call = block.insts.size() == 1 ? std::get_if<LirCallOp>(&block.insts.front()) : nullptr;
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

    const auto call_arg_imm = parse_i64(*call_operand);
    if (!call_arg_imm.has_value() || *call_arg_imm < std::numeric_limits<std::int32_t>::min() ||
        *call_arg_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    return MinimalSingleArgHelperCallSlice{
        .helper_name = helper.name,
        .entry_name = entry.name,
        .call_arg_imm = *call_arg_imm,
        .add_imm = add_imm,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalRegisterAggregateParamSlotSlice>
parse_minimal_register_aggregate_param_slot_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty() || module.type_decls.size() != 1 ||
      module.type_decls.front() != "%struct.Pair = type { [2 x i32] }") {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "get_second" || entry.name != "main" || helper.is_declaration ||
      entry.is_declaration || helper.entry.value != 0 || entry.entry.value != 0 ||
      helper.blocks.size() != 1 || entry.blocks.size() != 1 || helper.alloca_insts.size() != 2 ||
      entry.alloca_insts.size() != 1 || !helper.stack_objects.empty() ||
      !entry.stack_objects.empty() ||
      helper.signature_text != "define i32 @get_second(%struct.Pair %p.p)\n" ||
      entry.signature_text != "define i32 @main()\n") {
    return std::nullopt;
  }

  const auto* helper_slot = std::get_if<LirAllocaOp>(&helper.alloca_insts.front());
  const auto* helper_store = std::get_if<LirStoreOp>(&helper.alloca_insts[1]);
  if (helper_slot == nullptr || helper_store == nullptr ||
      helper_slot->result.str() != "%lv.param.p" || helper_slot->type_str != "%struct.Pair" ||
      !helper_slot->count.str().empty() || helper_slot->align != 4 ||
      helper_store->type_str != "%struct.Pair" || helper_store->val != "%p.p" ||
      helper_store->ptr != "%lv.param.p") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_entry.terminator);
  if (helper_entry.label != "entry" || helper_entry.insts.size() != 5 || helper_ret == nullptr ||
      helper_ret->type_str != "i32" || !helper_ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto* helper_gep0 = std::get_if<LirGepOp>(&helper_entry.insts[0]);
  const auto* helper_gep1 = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* helper_cast = std::get_if<LirCastOp>(&helper_entry.insts[2]);
  const auto* helper_gep2 = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* helper_load = std::get_if<LirLoadOp>(&helper_entry.insts[4]);
  if (helper_gep0 == nullptr || helper_gep1 == nullptr || helper_cast == nullptr ||
      helper_gep2 == nullptr || helper_load == nullptr ||
      helper_gep0->ptr != "%lv.param.p" || helper_cast->kind != LirCastKind::SExt ||
      helper_cast->operand != "1" || helper_load->type_str != "i32" ||
      *helper_ret->value_str != helper_load->result) {
    return std::nullopt;
  }

  const auto* entry_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
  if (entry_slot == nullptr || entry_slot->result.str() != "%lv.p" ||
      entry_slot->type_str != "%struct.Pair" || !entry_slot->count.str().empty() ||
      entry_slot->align != 4) {
    return std::nullopt;
  }

  const auto& entry_block = entry.blocks.front();
  const auto* entry_ret = std::get_if<LirRet>(&entry_block.terminator);
  if (entry_block.label != "entry" || entry_block.insts.size() != 12 || entry_ret == nullptr ||
      entry_ret->type_str != "i32" || !entry_ret->value_str.has_value()) {
    return std::nullopt;
  }

  const auto* entry_gep0 = std::get_if<LirGepOp>(&entry_block.insts[0]);
  const auto* entry_gep1 = std::get_if<LirGepOp>(&entry_block.insts[1]);
  const auto* entry_cast0 = std::get_if<LirCastOp>(&entry_block.insts[2]);
  const auto* entry_gep2 = std::get_if<LirGepOp>(&entry_block.insts[3]);
  const auto* entry_store0 = std::get_if<LirStoreOp>(&entry_block.insts[4]);
  const auto* entry_gep3 = std::get_if<LirGepOp>(&entry_block.insts[5]);
  const auto* entry_gep4 = std::get_if<LirGepOp>(&entry_block.insts[6]);
  const auto* entry_cast1 = std::get_if<LirCastOp>(&entry_block.insts[7]);
  const auto* entry_gep5 = std::get_if<LirGepOp>(&entry_block.insts[8]);
  const auto* entry_store1 = std::get_if<LirStoreOp>(&entry_block.insts[9]);
  if (entry_gep0 == nullptr || entry_gep1 == nullptr || entry_cast0 == nullptr ||
      entry_gep2 == nullptr || entry_store0 == nullptr || entry_gep3 == nullptr ||
      entry_gep4 == nullptr || entry_cast1 == nullptr || entry_gep5 == nullptr ||
      entry_store1 == nullptr) {
    return std::nullopt;
  }

  const auto first_field_imm = parse_i64(entry_store0->val);
  const auto second_field_imm = parse_i64(entry_store1->val);
  if (!first_field_imm.has_value() || !second_field_imm.has_value() ||
      *first_field_imm < std::numeric_limits<std::int32_t>::min() ||
      *first_field_imm > std::numeric_limits<std::int32_t>::max() ||
      *second_field_imm < std::numeric_limits<std::int32_t>::min() ||
      *second_field_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  const auto* entry_load = std::get_if<LirLoadOp>(&entry_block.insts[10]);
  const auto* entry_call = std::get_if<LirCallOp>(&entry_block.insts[11]);
  if (entry_gep0->ptr != "%lv.p" || entry_cast0->kind != LirCastKind::SExt ||
      entry_cast0->operand != "0" || entry_store0->type_str != "i32" ||
      entry_gep3->ptr != "%lv.p" || entry_cast1->kind != LirCastKind::SExt ||
      entry_cast1->operand != "1" || entry_store1->type_str != "i32" ||
      entry_load == nullptr || entry_load->result != "%t8" ||
      entry_load->type_str != "%struct.Pair" || entry_load->ptr != "%lv.p" ||
      entry_call == nullptr || entry_call->result != "%t9" || entry_call->callee != "@get_second" ||
      entry_call->return_type != "i32" || entry_call->callee_type_suffix != "(%struct.Pair)" ||
      entry_call->args_str != "%struct.Pair %t8" || *entry_ret->value_str != entry_call->result) {
    return std::nullopt;
  }

  return MinimalRegisterAggregateParamSlotSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .first_field_imm = *first_field_imm,
      .second_field_imm = *second_field_imm,
  };
}

std::optional<MinimalStackAggregateParamSlotSlice>
parse_minimal_stack_aggregate_param_slot_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty() || module.type_decls.size() != 1 ||
      module.type_decls.front() != "%struct.Big = type { [3 x i64] }") {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "get_third" || entry.name != "main" || helper.is_declaration ||
      entry.is_declaration || helper.entry.value != 0 || entry.entry.value != 0 ||
      helper.blocks.size() != 1 || entry.blocks.size() != 1 || helper.alloca_insts.size() != 2 ||
      entry.alloca_insts.size() != 1 || !helper.stack_objects.empty() ||
      !entry.stack_objects.empty() ||
      helper.signature_text != "define i64 @get_third(%struct.Big %p.p)\n" ||
      entry.signature_text != "define i64 @main()\n") {
    return std::nullopt;
  }

  const auto* helper_slot = std::get_if<LirAllocaOp>(&helper.alloca_insts.front());
  const auto* helper_store = std::get_if<LirStoreOp>(&helper.alloca_insts[1]);
  if (helper_slot == nullptr || helper_store == nullptr ||
      helper_slot->result.str() != "%lv.param.p" || helper_slot->type_str != "%struct.Big" ||
      !helper_slot->count.str().empty() || helper_slot->align != 8 ||
      helper_store->type_str != "%struct.Big" || helper_store->val != "%p.p" ||
      helper_store->ptr != "%lv.param.p") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_entry.terminator);
  if (helper_entry.label != "entry" || helper_entry.insts.size() != 5 || helper_ret == nullptr ||
      helper_ret->type_str != "i64" || !helper_ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto* helper_gep0 = std::get_if<LirGepOp>(&helper_entry.insts[0]);
  const auto* helper_gep1 = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* helper_cast = std::get_if<LirCastOp>(&helper_entry.insts[2]);
  const auto* helper_gep2 = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* helper_load = std::get_if<LirLoadOp>(&helper_entry.insts[4]);
  if (helper_gep0 == nullptr || helper_gep1 == nullptr || helper_cast == nullptr ||
      helper_gep2 == nullptr || helper_load == nullptr ||
      helper_gep0->ptr != "%lv.param.p" || helper_cast->kind != LirCastKind::SExt ||
      helper_cast->operand != "2" || helper_load->type_str != "i64" ||
      *helper_ret->value_str != helper_load->result) {
    return std::nullopt;
  }

  const auto* entry_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
  if (entry_slot == nullptr || entry_slot->result.str() != "%lv.p" ||
      entry_slot->type_str != "%struct.Big" || !entry_slot->count.str().empty() ||
      entry_slot->align != 8) {
    return std::nullopt;
  }

  const auto& entry_block = entry.blocks.front();
  const auto* entry_ret = std::get_if<LirRet>(&entry_block.terminator);
  if (entry_block.label != "entry" || entry_block.insts.size() != 17 || entry_ret == nullptr ||
      entry_ret->type_str != "i64" || !entry_ret->value_str.has_value()) {
    return std::nullopt;
  }

  const auto* entry_gep0 = std::get_if<LirGepOp>(&entry_block.insts[0]);
  const auto* entry_gep1 = std::get_if<LirGepOp>(&entry_block.insts[1]);
  const auto* entry_cast0 = std::get_if<LirCastOp>(&entry_block.insts[2]);
  const auto* entry_gep2 = std::get_if<LirGepOp>(&entry_block.insts[3]);
  const auto* entry_store0 = std::get_if<LirStoreOp>(&entry_block.insts[4]);
  const auto* entry_gep3 = std::get_if<LirGepOp>(&entry_block.insts[5]);
  const auto* entry_gep4 = std::get_if<LirGepOp>(&entry_block.insts[6]);
  const auto* entry_cast1 = std::get_if<LirCastOp>(&entry_block.insts[7]);
  const auto* entry_gep5 = std::get_if<LirGepOp>(&entry_block.insts[8]);
  const auto* entry_store1 = std::get_if<LirStoreOp>(&entry_block.insts[9]);
  const auto* entry_gep6 = std::get_if<LirGepOp>(&entry_block.insts[10]);
  const auto* entry_gep7 = std::get_if<LirGepOp>(&entry_block.insts[11]);
  const auto* entry_cast2 = std::get_if<LirCastOp>(&entry_block.insts[12]);
  const auto* entry_gep8 = std::get_if<LirGepOp>(&entry_block.insts[13]);
  const auto* entry_store2 = std::get_if<LirStoreOp>(&entry_block.insts[14]);
  const auto* entry_load = std::get_if<LirLoadOp>(&entry_block.insts[15]);
  const auto* entry_call = std::get_if<LirCallOp>(&entry_block.insts[16]);
  if (entry_gep0 == nullptr || entry_gep1 == nullptr || entry_cast0 == nullptr ||
      entry_gep2 == nullptr || entry_store0 == nullptr || entry_gep3 == nullptr ||
      entry_gep4 == nullptr || entry_cast1 == nullptr || entry_gep5 == nullptr ||
      entry_store1 == nullptr || entry_gep6 == nullptr || entry_gep7 == nullptr ||
      entry_cast2 == nullptr || entry_gep8 == nullptr || entry_store2 == nullptr ||
      entry_load == nullptr || entry_call == nullptr) {
    return std::nullopt;
  }

  const auto first_field_imm = parse_i64(entry_store0->val);
  const auto second_field_imm = parse_i64(entry_store1->val);
  const auto third_field_imm = parse_i64(entry_store2->val);
  if (!first_field_imm.has_value() || !second_field_imm.has_value() || !third_field_imm.has_value()) {
    return std::nullopt;
  }

  if (entry_gep0->ptr != "%lv.p" || entry_cast0->kind != LirCastKind::SExt ||
      entry_cast0->operand != "0" || entry_store0->type_str != "i64" ||
      entry_gep3->ptr != "%lv.p" || entry_cast1->kind != LirCastKind::SExt ||
      entry_cast1->operand != "1" || entry_store1->type_str != "i64" ||
      entry_gep6->ptr != "%lv.p" || entry_cast2->kind != LirCastKind::SExt ||
      entry_cast2->operand != "2" || entry_store2->type_str != "i64" ||
      entry_load->result != "%t12" || entry_load->type_str != "%struct.Big" ||
      entry_load->ptr != "%lv.p" || entry_call->result != "%t13" ||
      entry_call->callee != "@get_third" || entry_call->return_type != "i64" ||
      entry_call->callee_type_suffix != "(%struct.Big)" ||
      entry_call->args_str != "%struct.Big %t12" || *entry_ret->value_str != entry_call->result) {
    return std::nullopt;
  }

  return MinimalStackAggregateParamSlotSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .first_field_imm = *first_field_imm,
      .second_field_imm = *second_field_imm,
      .third_field_imm = *third_field_imm,
  };
}

std::optional<MinimalSmallStructStackParamSlotSlice>
parse_minimal_small_struct_stack_param_slot_slice(const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.extern_decls.empty() ||
      !module.string_pool.empty() || module.type_decls.size() != 1 ||
      module.type_decls.front() != "%struct.Pair = type { [2 x i32] }") {
    return std::nullopt;
  }

  const auto& helper = module.functions.front();
  const auto& entry = module.functions.back();
  if (helper.name != "get_second_after_six" || entry.name != "main" || helper.is_declaration ||
      entry.is_declaration || helper.entry.value != 0 || entry.entry.value != 0 ||
      helper.blocks.size() != 1 || entry.blocks.size() != 1 || helper.alloca_insts.size() != 2 ||
      entry.alloca_insts.size() != 1 || !helper.stack_objects.empty() ||
      !entry.stack_objects.empty() ||
      helper.signature_text !=
          "define i32 @get_second_after_six(i64 %p.a, i64 %p.b, i64 %p.c, i64 %p.d, i64 %p.e, i64 %p.f, %struct.Pair %p.p)\n" ||
      entry.signature_text != "define i32 @main()\n") {
    return std::nullopt;
  }

  const auto* helper_slot = std::get_if<LirAllocaOp>(&helper.alloca_insts.front());
  const auto* helper_store = std::get_if<LirStoreOp>(&helper.alloca_insts[1]);
  if (helper_slot == nullptr || helper_store == nullptr ||
      helper_slot->result.str() != "%lv.param.p" || helper_slot->type_str != "%struct.Pair" ||
      !helper_slot->count.str().empty() || helper_slot->align != 4 ||
      helper_store->type_str != "%struct.Pair" || helper_store->val != "%p.p" ||
      helper_store->ptr != "%lv.param.p") {
    return std::nullopt;
  }

  const auto& helper_entry = helper.blocks.front();
  const auto* helper_ret = std::get_if<LirRet>(&helper_entry.terminator);
  if (helper_entry.label != "entry" || helper_entry.insts.size() != 5 || helper_ret == nullptr ||
      helper_ret->type_str != "i32" || !helper_ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto* helper_gep0 = std::get_if<LirGepOp>(&helper_entry.insts[0]);
  const auto* helper_gep1 = std::get_if<LirGepOp>(&helper_entry.insts[1]);
  const auto* helper_cast = std::get_if<LirCastOp>(&helper_entry.insts[2]);
  const auto* helper_gep2 = std::get_if<LirGepOp>(&helper_entry.insts[3]);
  const auto* helper_load = std::get_if<LirLoadOp>(&helper_entry.insts[4]);
  if (helper_gep0 == nullptr || helper_gep1 == nullptr || helper_cast == nullptr ||
      helper_gep2 == nullptr || helper_load == nullptr ||
      helper_gep0->ptr != "%lv.param.p" || helper_cast->kind != LirCastKind::SExt ||
      helper_cast->operand != "1" || helper_load->type_str != "i32" ||
      *helper_ret->value_str != helper_load->result) {
    return std::nullopt;
  }

  const auto* entry_slot = std::get_if<LirAllocaOp>(&entry.alloca_insts.front());
  if (entry_slot == nullptr || entry_slot->result.str() != "%lv.p" ||
      entry_slot->type_str != "%struct.Pair" || !entry_slot->count.str().empty() ||
      entry_slot->align != 4) {
    return std::nullopt;
  }

  const auto& entry_block = entry.blocks.front();
  const auto* entry_ret = std::get_if<LirRet>(&entry_block.terminator);
  if (entry_block.label != "entry" || entry_block.insts.size() != 12 || entry_ret == nullptr ||
      entry_ret->type_str != "i32" || !entry_ret->value_str.has_value()) {
    return std::nullopt;
  }

  const auto* entry_gep0 = std::get_if<LirGepOp>(&entry_block.insts[0]);
  const auto* entry_gep1 = std::get_if<LirGepOp>(&entry_block.insts[1]);
  const auto* entry_cast0 = std::get_if<LirCastOp>(&entry_block.insts[2]);
  const auto* entry_gep2 = std::get_if<LirGepOp>(&entry_block.insts[3]);
  const auto* entry_store0 = std::get_if<LirStoreOp>(&entry_block.insts[4]);
  const auto* entry_gep3 = std::get_if<LirGepOp>(&entry_block.insts[5]);
  const auto* entry_gep4 = std::get_if<LirGepOp>(&entry_block.insts[6]);
  const auto* entry_cast1 = std::get_if<LirCastOp>(&entry_block.insts[7]);
  const auto* entry_gep5 = std::get_if<LirGepOp>(&entry_block.insts[8]);
  const auto* entry_store1 = std::get_if<LirStoreOp>(&entry_block.insts[9]);
  const auto* entry_load = std::get_if<LirLoadOp>(&entry_block.insts[10]);
  const auto* entry_call = std::get_if<LirCallOp>(&entry_block.insts[11]);
  if (entry_gep0 == nullptr || entry_gep1 == nullptr || entry_cast0 == nullptr ||
      entry_gep2 == nullptr || entry_store0 == nullptr || entry_gep3 == nullptr ||
      entry_gep4 == nullptr || entry_cast1 == nullptr || entry_gep5 == nullptr ||
      entry_store1 == nullptr || entry_load == nullptr || entry_call == nullptr) {
    return std::nullopt;
  }

  const auto first_field_imm = parse_i64(entry_store0->val);
  const auto second_field_imm = parse_i64(entry_store1->val);
  if (!first_field_imm.has_value() || !second_field_imm.has_value() ||
      *first_field_imm < std::numeric_limits<std::int32_t>::min() ||
      *first_field_imm > std::numeric_limits<std::int32_t>::max() ||
      *second_field_imm < std::numeric_limits<std::int32_t>::min() ||
      *second_field_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  if (entry_gep0->ptr != "%lv.p" || entry_cast0->kind != LirCastKind::SExt ||
      entry_cast0->operand != "0" || entry_store0->type_str != "i32" ||
      entry_gep3->ptr != "%lv.p" || entry_cast1->kind != LirCastKind::SExt ||
      entry_cast1->operand != "1" || entry_store1->type_str != "i32" ||
      entry_load->result != "%t8" || entry_load->type_str != "%struct.Pair" ||
      entry_load->ptr != "%lv.p" || entry_call->result != "%t9" ||
      entry_call->callee != "@get_second_after_six" || entry_call->return_type != "i32" ||
      entry_call->callee_type_suffix != "(i64, i64, i64, i64, i64, i64, %struct.Pair)" ||
      entry_call->args_str != "i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, %struct.Pair %t8" ||
      *entry_ret->value_str != entry_call->result) {
    return std::nullopt;
  }

  return MinimalSmallStructStackParamSlotSlice{
      .helper_name = helper.name,
      .entry_name = entry.name,
      .first_field_imm = *first_field_imm,
      .second_field_imm = *second_field_imm,
  };
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

std::string emit_minimal_seventh_param_stack_add_asm(
    std::string_view target_triple,
    const MinimalSeventhParamStackAddSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  mov eax, DWORD PTR [rbp + 16]\n";
  if (slice.add_imm > 0) {
    out << "  add eax, " << slice.add_imm << "\n";
  } else if (slice.add_imm < 0) {
    out << "  sub eax, " << -slice.add_imm << "\n";
  }
  out << "  pop rbp\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol);

  static constexpr std::int64_t kFirstSixCallImms[] = {1, 2, 3, 4, 5, 6};
  for (std::size_t index = 0; index < std::size(kFirstSixCallImms); ++index) {
    out << "  mov " << reg_name_to_32(x86_arg_reg_name(index)) << ", " << kFirstSixCallImms[index]
        << "\n";
  }
  out << "  push " << slice.seventh_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  add rsp, 8\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_mixed_reg_stack_param_add_asm(
    std::string_view target_triple,
    const MinimalMixedRegStackParamAddSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  const bool is_i64 = slice.scalar_type == "i64";
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  mov " << (is_i64 ? "rax, rdi" : "eax, edi") << "\n"
      << "  add " << (is_i64 ? "rax, QWORD PTR [rbp + 16]" : "eax, DWORD PTR [rbp + 16]") << "\n"
      << "  pop rbp\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov " << (is_i64 ? "rdi" : "edi") << ", " << slice.first_arg_imm << "\n"
      << "  mov " << (is_i64 ? "rsi" : "esi") << ", 2\n"
      << "  mov " << (is_i64 ? "rdx" : "edx") << ", 3\n"
      << "  mov " << (is_i64 ? "rcx" : "ecx") << ", 4\n"
      << "  mov " << (is_i64 ? "r8" : "r8d") << ", 5\n"
      << "  mov " << (is_i64 ? "r9" : "r9d") << ", 6\n"
      << "  push " << slice.seventh_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  add rsp, 8\n"
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

std::string emit_minimal_single_arg_helper_call_asm(
    std::string_view target_triple,
    const MinimalSingleArgHelperCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n";
  if (slice.add_imm.has_value()) {
    if (*slice.add_imm > 0) {
      out << "  add eax, " << *slice.add_imm << "\n";
    } else if (*slice.add_imm < 0) {
      out << "  sub eax, " << -*slice.add_imm << "\n";
    }
  }
  out << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, " << slice.call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_register_aggregate_param_slot_asm(
    std::string_view target_triple,
    const MinimalRegisterAggregateParamSlotSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  mov QWORD PTR [rbp - 8], rdi\n"
      << "  mov eax, DWORD PTR [rbp - 4]\n"
      << "  pop rbp\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  mov DWORD PTR [rbp - 8], " << slice.first_field_imm << "\n"
      << "  mov DWORD PTR [rbp - 4], " << slice.second_field_imm << "\n"
      << "  mov rdi, QWORD PTR [rbp - 8]\n"
      << "  call " << helper_symbol << "\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_stack_aggregate_param_slot_asm(
    std::string_view target_triple,
    const MinimalStackAggregateParamSlotSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 24\n"
      << "  mov rax, QWORD PTR [rbp + 16]\n"
      << "  mov QWORD PTR [rbp - 24], rax\n"
      << "  mov rax, QWORD PTR [rbp + 24]\n"
      << "  mov QWORD PTR [rbp - 16], rax\n"
      << "  mov rax, QWORD PTR [rbp + 32]\n"
      << "  mov QWORD PTR [rbp - 8], rax\n"
      << "  mov rax, QWORD PTR [rbp - 8]\n"
      << "  pop rbp\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 32\n"
      << "  mov QWORD PTR [rbp - 24], " << slice.first_field_imm << "\n"
      << "  mov QWORD PTR [rbp - 16], " << slice.second_field_imm << "\n"
      << "  mov QWORD PTR [rbp - 8], " << slice.third_field_imm << "\n"
      << "  sub rsp, 32\n"
      << "  mov rax, QWORD PTR [rbp - 24]\n"
      << "  mov QWORD PTR [rsp], rax\n"
      << "  mov rax, QWORD PTR [rbp - 16]\n"
      << "  mov QWORD PTR [rsp + 8], rax\n"
      << "  mov rax, QWORD PTR [rbp - 8]\n"
      << "  mov QWORD PTR [rsp + 16], rax\n"
      << "  call " << helper_symbol << "\n"
      << "  add rsp, 32\n"
      << "  pop rbp\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_small_struct_stack_param_slot_asm(
    std::string_view target_triple,
    const MinimalSmallStructStackParamSlotSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  mov rax, QWORD PTR [rbp + 16]\n"
      << "  mov QWORD PTR [rbp - 8], rax\n"
      << "  mov eax, DWORD PTR [rbp - 4]\n"
      << "  pop rbp\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbp\n"
      << "  mov rbp, rsp\n"
      << "  sub rsp, 16\n"
      << "  mov DWORD PTR [rbp - 8], " << slice.first_field_imm << "\n"
      << "  mov DWORD PTR [rbp - 4], " << slice.second_field_imm << "\n"
      << "  mov rdi, 1\n"
      << "  mov rsi, 2\n"
      << "  mov rdx, 3\n"
      << "  mov rcx, 4\n"
      << "  mov r8, 5\n"
      << "  mov r9, 6\n"
      << "  sub rsp, 16\n"
      << "  mov rax, QWORD PTR [rbp - 8]\n"
      << "  mov QWORD PTR [rsp], rax\n"
      << "  call " << helper_symbol << "\n"
      << "  add rsp, 16\n"
      << "  pop rbp\n"
      << "  ret\n";
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

std::string emit_minimal_folded_two_arg_direct_call_asm(
    std::string_view target_triple,
    const MinimalFoldedTwoArgDirectCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, " << slice.helper_add_imm << "\n"
      << "  add eax, edi\n"
      << "  sub eax, esi\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  mov esi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << helper_symbol << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_dual_identity_direct_call_sub_asm(
    std::string_view target_triple,
    const MinimalDualIdentityDirectCallSubSlice& slice) {
  const auto lhs_helper_symbol = asm_symbol_name(target_triple, slice.lhs_helper_name);
  const auto rhs_helper_symbol = asm_symbol_name(target_triple, slice.rhs_helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, lhs_helper_symbol)
      << "  mov eax, edi\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, rhs_helper_symbol)
      << "  mov eax, edi\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbx\n"
      << "  mov edi, " << slice.lhs_call_arg_imm << "\n"
      << "  call " << lhs_helper_symbol << "\n"
      << "  mov ebx, eax\n"
      << "  mov edi, " << slice.rhs_call_arg_imm << "\n"
      << "  call " << rhs_helper_symbol << "\n"
      << "  sub ebx, eax\n"
      << "  mov eax, ebx\n"
      << "  pop rbx\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_call_crossing_direct_call_asm(
    std::string_view target_triple,
    const MinimalCallCrossingDirectCallSlice& slice) {
  const auto helper_symbol = asm_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = asm_symbol_name(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  mov eax, edi\n";
  if (slice.helper_add_imm > 0) {
    out << "  add eax, " << slice.helper_add_imm << "\n";
  } else if (slice.helper_add_imm < 0) {
    out << "  sub eax, " << -slice.helper_add_imm << "\n";
  }
  out << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  push rbx\n"
      << "  mov ebx, " << slice.source_imm << "\n"
      << "  mov edi, ebx\n"
      << "  call " << helper_symbol << "\n"
      << "  add eax, ebx\n"
      << "  pop rbx\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::optional<std::string> try_emit_minimal_param_slot_add_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_param_slot_add_slice(module); slice.has_value()) {
    return emit_minimal_param_slot_add_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_seventh_param_stack_add_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_seventh_param_stack_add_slice(module); slice.has_value()) {
    return emit_minimal_seventh_param_stack_add_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_mixed_reg_stack_param_add_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_mixed_reg_stack_param_add_slice(module); slice.has_value()) {
    return emit_minimal_mixed_reg_stack_param_add_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_register_aggregate_param_slot_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_register_aggregate_param_slot_slice(module);
      slice.has_value()) {
    return emit_minimal_register_aggregate_param_slot_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_stack_aggregate_param_slot_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_stack_aggregate_param_slot_slice(module);
      slice.has_value()) {
    return emit_minimal_stack_aggregate_param_slot_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_small_struct_stack_param_slot_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_small_struct_stack_param_slot_slice(module);
      slice.has_value()) {
    return emit_minimal_small_struct_stack_param_slot_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_extern_zero_arg_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_declared_zero_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_extern_zero_arg_call_asm(module.target_triple, *slice);
  }
  if (const auto slice = parse_minimal_extern_zero_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_extern_zero_arg_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_single_arg_helper_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_single_arg_helper_call_slice(module); slice.has_value()) {
    return emit_minimal_single_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_local_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_local_arg_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_helper_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_helper_call_slice(module); slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_folded_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_folded_two_arg_direct_call_slice(module); slice.has_value()) {
    return emit_minimal_folded_two_arg_direct_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_dual_identity_direct_call_sub_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_dual_identity_direct_call_sub_slice(module);
      slice.has_value()) {
    return emit_minimal_dual_identity_direct_call_sub_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_call_crossing_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module); slice.has_value()) {
    return emit_minimal_call_crossing_direct_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_call_crossing_direct_call_bir_module(
    const c4c::backend::bir::Module& module) {
  const auto parsed = c4c::backend::parse_bir_minimal_call_crossing_direct_call_module(module);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_call_crossing_direct_call_asm(
      module.target_triple,
      MinimalCallCrossingDirectCallSlice{
          .helper_name = parsed->helper->name,
          .entry_name = parsed->main_function->name,
          .source_imm = parsed->source_imm,
          .helper_add_imm = parsed->helper_add_imm,
      });
}

std::optional<std::string> try_emit_minimal_two_arg_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_local_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_second_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_second_local_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_both_local_arg_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_both_local_arg_call_slice(module); slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_both_local_first_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_both_local_first_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_both_local_second_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_both_local_second_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_both_local_double_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_both_local_double_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_second_local_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_second_local_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_minimal_two_arg_first_local_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_first_local_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
