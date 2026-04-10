#include "x86_codegen.hpp"

#include "../../backend.hpp"
#include "../../lowering/call_decode.hpp"
#include "../../../codegen/lir/ir.hpp"

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

std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name) {
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

std::string emit_minimal_param_slot_add_asm(std::string_view target_triple,
                                            const MinimalParamSlotAddSlice& slice) {
  const auto helper_symbol = direct_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = direct_symbol_name(target_triple, slice.entry_name);
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
  const auto extern_symbol = direct_symbol_name(target_triple, slice.extern_name);
  const auto function_symbol = direct_symbol_name(target_triple, slice.function_name);
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
  const auto helper_symbol = direct_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = direct_symbol_name(target_triple, slice.entry_name);
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

std::string emit_minimal_two_arg_helper_call_asm(
    std::string_view target_triple,
    const MinimalTwoArgHelperCallSlice& slice) {
  const auto helper_symbol = direct_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = direct_symbol_name(target_triple, slice.entry_name);
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

}  // namespace

std::optional<std::string> try_emit_minimal_param_slot_add_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_param_slot_add_slice(module); slice.has_value()) {
    return emit_minimal_param_slot_add_asm(module.target_triple, *slice);
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

std::optional<std::string> try_emit_minimal_two_arg_second_local_rewrite_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto slice = parse_minimal_two_arg_second_local_rewrite_call_slice(module);
      slice.has_value()) {
    return emit_minimal_two_arg_helper_call_asm(module.target_triple, *slice);
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86
