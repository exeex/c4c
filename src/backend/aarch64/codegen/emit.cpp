#include "emit.hpp"

#include "../../lir_adapter.hpp"
#include "../../generation.hpp"
#include "../../ir_printer.hpp"
#include "../../stack_layout/analysis.hpp"
#include "../../stack_layout/regalloc_helpers.hpp"
#include "../../stack_layout/slot_assignment.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <vector>

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

std::optional<std::string_view> strip_typed_operand_prefix(std::string_view operand,
                                                           std::string_view type_prefix);

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

constexpr std::array<c4c::backend::PhysReg, 9> kAarch64CalleeSavedRegs = {
    c4c::backend::PhysReg{20}, c4c::backend::PhysReg{21}, c4c::backend::PhysReg{22},
    c4c::backend::PhysReg{23}, c4c::backend::PhysReg{24}, c4c::backend::PhysReg{25},
    c4c::backend::PhysReg{26}, c4c::backend::PhysReg{27}, c4c::backend::PhysReg{28},
};

constexpr std::array<c4c::backend::PhysReg, 2> kAarch64CallerSavedRegs = {
    c4c::backend::PhysReg{13},
    c4c::backend::PhysReg{14},
};

void prune_dead_entry_allocas(c4c::codegen::lir::LirFunction& function) {
  c4c::backend::RegAllocConfig config;
  config.available_regs.assign(kAarch64CalleeSavedRegs.begin(), kAarch64CalleeSavedRegs.end());
  config.caller_saved_regs.assign(kAarch64CallerSavedRegs.begin(), kAarch64CallerSavedRegs.end());

  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
  const std::vector<c4c::backend::PhysReg> callee_saved(kAarch64CalleeSavedRegs.begin(),
                                                        kAarch64CalleeSavedRegs.end());
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, callee_saved);
  const auto plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(function, analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(function, plans);
}

c4c::codegen::lir::LirModule prepare_module_for_fallback(
    const c4c::codegen::lir::LirModule& module) {
  auto prepared = module;
  for (auto& function : prepared.functions) {
    prune_dead_entry_allocas(function);
  }
  return prepared;
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

std::optional<std::int64_t> parse_minimal_return_sub_imm(
    const c4c::backend::BackendModule& module) {
  if (!is_minimal_single_function_asm_slice(module)) {
    return std::nullopt;
  }

  const auto& block = module.functions.front().blocks.front();
  if (block.insts.size() != 1 ||
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
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

std::optional<std::int64_t> parse_minimal_lir_return_sub_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
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
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
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
  if (function.is_declaration || function.name != "main" ||
      function.signature_text.find("i32 @main(") == std::string::npos ||
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

std::optional<std::int64_t> parse_minimal_lir_single_scalar_countdown_imm(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
      function.entry.value != 0 || function.blocks.empty() ||
      function.alloca_insts.size() != 1) {
    return std::nullopt;
  }

  const auto* scalar_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (scalar_alloca == nullptr || scalar_alloca->type_str != "i32" ||
      scalar_alloca->result.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  std::size_t static_store_count = 0;
  std::size_t static_cmp_count = 0;
  std::size_t static_sub_count = 0;
  std::size_t static_ret_count = 0;
  std::optional<std::int64_t> initial_value;
  const auto parse_typed_i64 = [](std::string_view value)
      -> std::optional<std::int64_t> {
    const auto space = value.find(' ');
    if (space == std::string_view::npos) {
      return parse_i64(value);
    }
    return parse_i64(value.substr(space + 1));
  };

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        if (store->type_str != "i32" || store->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        if (block.label == "entry") {
          initial_value = parse_typed_i64(store->val);
          if (!initial_value.has_value()) {
            return std::nullopt;
          }
        }
        ++static_store_count;
        continue;
      }

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        if (load->type_str != "i32" || load->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        continue;
      }

      if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
        const auto bin_opcode = bin->opcode.typed();
        if (!bin_opcode || *bin_opcode != LirBinaryOpcode::Sub || bin->type_str != "i32") {
          return std::nullopt;
        }
        ++static_sub_count;
        continue;
      }

      if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
        if (cmp->is_float || cmp->type_str != "i32" ||
            cmp->predicate.typed() != LirCmpPredicate::Ne) {
          return std::nullopt;
        }
        ++static_cmp_count;
        continue;
      }

      return std::nullopt;
    }

    if (std::holds_alternative<LirRet>(block.terminator)) {
      ++static_ret_count;
      continue;
    }
    if (std::holds_alternative<LirBr>(block.terminator) ||
        std::holds_alternative<LirCondBr>(block.terminator)) {
      continue;
    }
    return std::nullopt;
  }

  if (!initial_value.has_value() || *initial_value < 0 || static_store_count < 2 ||
      static_cmp_count < 1 || static_sub_count < 1 || static_ret_count == 0 ||
      static_ret_count > 2) {
    return std::nullopt;
  }

  const auto max_steps = (static_cast<std::size_t>(*initial_value) + 1) *
                         (function.blocks.size() + 1);
  if (max_steps == 0) {
    return std::nullopt;
  }

  const auto resolve_i32 = [&](std::string_view value,
                               const std::unordered_map<std::string, std::int64_t>& values)
                              -> std::optional<std::int64_t> {
    const auto imm = parse_typed_i64(value);
    if (imm.has_value()) {
      return imm;
    }
    const auto it = values.find(std::string(value));
    if (it == values.end()) return std::nullopt;
    return it->second;
  };

  std::string current_label = function.blocks.front().label;
  std::int64_t scalar_value = *initial_value;
  bool initialized = true;

  for (std::size_t step = 0; step < max_steps; ++step) {
    const auto block_it = blocks_by_label.find(current_label);
    if (block_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    const auto& block = *block_it->second;

    std::unordered_map<std::string, std::int64_t> integer_values;
    std::unordered_map<std::string, bool> predicate_values;
    integer_values.clear();
    predicate_values.clear();

    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        if (store->type_str != "i32" || store->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        const auto value = resolve_i32(store->val, integer_values);
        if (!value.has_value()) return std::nullopt;
        scalar_value = *value;
        initialized = true;
        continue;
      }

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        if (load->type_str != "i32" || load->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        integer_values[load->result] = scalar_value;
        continue;
      }

      if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
        const auto lhs = resolve_i32(bin->lhs, integer_values);
        const auto rhs = resolve_i32(bin->rhs, integer_values);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }

        integer_values[bin->result] = *lhs - *rhs;
        continue;
      }

      if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
        if (cmp->type_str != "i32" || cmp->is_float ||
            cmp->predicate.typed() != LirCmpPredicate::Ne) {
          return std::nullopt;
        }

        const auto lhs = resolve_i32(cmp->lhs, integer_values);
        const auto rhs = resolve_i32(cmp->rhs, integer_values);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        predicate_values[cmp->result] = *lhs != *rhs;
        continue;
      }

      return std::nullopt;
    }

    if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
      if (!ret->value_str.has_value() || ret->type_str != "i32") {
        return std::nullopt;
      }
      if (ret->value_str == scalar_alloca->result) {
        if (!initialized) return std::nullopt;
        return scalar_value;
      }
      const auto ret_value = resolve_i32(*ret->value_str, integer_values);
      if (!ret_value.has_value()) {
        return std::nullopt;
      }
      return *ret_value;
    }

    if (const auto* br = std::get_if<LirBr>(&block.terminator)) {
      current_label = br->target_label;
      continue;
    }

    const auto* condbr = std::get_if<LirCondBr>(&block.terminator);
    if (condbr == nullptr) {
      return std::nullopt;
    }

    const auto pred_it = predicate_values.find(condbr->cond_name);
    if (pred_it == predicate_values.end()) {
      return std::nullopt;
    }
    current_label = pred_it->second ? condbr->true_label : condbr->false_label;
  }

  if (!initialized) return std::nullopt;
  return std::nullopt;
}

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
  if (function.is_declaration || function.name != "main" ||
      function.signature_text.find("i32 @main(") == std::string::npos ||
      function.entry.value != 0 || function.blocks.empty()) {
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

struct MinimalCallCrossingDirectCallSlice {
  std::string callee_name;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
  std::string source_value;
  std::string call_result_value;
};

std::optional<std::int64_t> fold_minimal_direct_call_two_arg_callee_return(
    const c4c::backend::BackendFunction& callee_fn,
    std::int64_t arg0_imm,
    std::int64_t arg1_imm) {
  if (callee_fn.signature.params.size() != 2 ||
      callee_fn.signature.params[0].type_str != "i32" ||
      callee_fn.signature.params[1].type_str != "i32" ||
      callee_fn.signature.params[0].name.empty() ||
      callee_fn.signature.params[1].name.empty() || callee_fn.signature.is_vararg ||
      callee_fn.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = callee_fn.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::int64_t> values;
  values[callee_fn.signature.params[0].name] = arg0_imm;
  values[callee_fn.signature.params[1].name] = arg1_imm;

  const auto resolve_i32_operand = [&](std::string_view operand)
      -> std::optional<std::int64_t> {
    const auto imm = parse_i64(operand);
    if (imm.has_value()) {
      return imm;
    }
    auto it = values.find(std::string(operand));
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (const auto& inst : block.insts) {
    const auto* bin =
        std::get_if<c4c::backend::BackendBinaryInst>(&inst);
    if (bin == nullptr || bin->type_str != "i32" || bin->result.empty()) {
      return std::nullopt;
    }

    const auto lhs = resolve_i32_operand(bin->lhs);
    const auto rhs = resolve_i32_operand(bin->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    std::int64_t value = 0;
    if (bin->opcode == c4c::backend::BackendBinaryOpcode::Add) {
      value = *lhs + *rhs;
    } else if (bin->opcode == c4c::backend::BackendBinaryOpcode::Sub) {
      value = *lhs - *rhs;
    } else {
      return std::nullopt;
    }

    values[bin->result] = value;
  }

  return resolve_i32_operand(*block.terminator.value);
}

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

struct MinimalCountdownLoopSlice {
  std::int64_t initial_imm = 0;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
};

struct MinimalScalarGlobalLoadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  int align_bytes = 4;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  int align_bytes = 4;
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

struct MinimalLocalArraySlice {
  std::int64_t store0_imm = 0;
  std::int64_t store1_imm = 0;
};

struct MinimalStringLiteralCharSlice {
  std::string pool_name;
  std::string raw_bytes;
  std::int64_t byte_index = 0;
  c4c::codegen::lir::LirCastKind extend_kind = c4c::codegen::lir::LirCastKind::ZExt;
};

const c4c::codegen::lir::LirFunction* find_lir_function(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  for (const auto& function : module.functions) {
    if (function.name == name) return &function;
  }
  return nullptr;
}

c4c::backend::RegAllocIntegrationResult run_shared_aarch64_regalloc(
    const c4c::codegen::lir::LirFunction& function) {
  c4c::backend::RegAllocConfig config;
  config.available_regs.assign(kAarch64CalleeSavedRegs.begin(), kAarch64CalleeSavedRegs.end());
  config.caller_saved_regs.assign(kAarch64CallerSavedRegs.begin(), kAarch64CallerSavedRegs.end());
  return c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
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
      string_const.pool_name,
      string_const.raw_bytes,
      *byte_index,
      extend->kind,
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
      load->address.byte_offset < 0 || load->address.byte_offset > 4095) {
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
      load->extension == c4c::backend::BackendLoadExtension::SignExtend
          ? c4c::codegen::lir::LirCastKind::SExt
          : c4c::codegen::lir::LirCastKind::ZExt,
  };
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
      stores.push_back({ptr->second, *imm});
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
      if (add->opcode != "add" || add->type_str != "i32") {
        return std::nullopt;
      }
      add_result = add->result;
      continue;
    }

    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32" ||
      *ret->value_str != add_result) {
    return std::nullopt;
  }

  if (stores.size() != 2 || loads.size() != 2 ||
      stores[0].first != 0 || stores[1].first != 4 ||
      loads[0] != 0 || loads[1] != 4) {
    return std::nullopt;
  }

  return MinimalLocalArraySlice{stores[0].second, stores[1].second};
}

std::string gp_reg_name(c4c::backend::PhysReg reg, bool is_32bit) {
  return std::string(is_32bit ? "w" : "x") + std::to_string(reg.index);
}

std::int64_t aligned_call_frame_size(std::size_t saved_regs) {
  const std::size_t raw_size = (saved_regs + 1) * sizeof(std::uint64_t);
  return static_cast<std::int64_t>((raw_size + 15) & ~std::size_t{15});
}

std::optional<MinimalConditionalReturnSlice> parse_minimal_conditional_return_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto* function = find_function(module, "main");
  if (function == nullptr || function->is_declaration ||
      function->signature.linkage != "define" ||
      function->signature.return_type != "i32" ||
      !function->signature.params.empty() || function->signature.is_vararg ||
      function->blocks.size() != 3) {
    return std::nullopt;
  }

  const auto& entry = function->blocks[0];
  const auto& true_block = function->blocks[1];
  const auto& false_block = function->blocks[2];
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
       *cmp0_predicate != LirCmpPredicate::Ult && *cmp0_predicate != LirCmpPredicate::Ule &&
       *cmp0_predicate != LirCmpPredicate::Ugt && *cmp0_predicate != LirCmpPredicate::Uge &&
       *cmp0_predicate != LirCmpPredicate::Eq && *cmp0_predicate != LirCmpPredicate::Ne) ||
      cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" || cmp1->lhs != cast->result ||
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

  const auto* function = find_function(module, "main");
  if (function == nullptr || function->is_declaration ||
      function->signature.linkage != "define" ||
      function->signature.return_type != "i32" ||
      !function->signature.params.empty() || function->signature.is_vararg ||
      function->blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function->blocks[0];
  const auto& loop = function->blocks[1];
  const auto& body = function->blocks[2];
  const auto& exit = function->blocks[3];
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
      phi->incoming[1].label != body.label ||
      cmp->predicate != c4c::backend::BackendComparePredicate::Ne ||
      cmp->result != loop.terminator.cond_name || cmp->type_str != "i32" ||
      cmp->lhs != phi->result || cmp->rhs != "0" ||
      dec->opcode != c4c::backend::BackendBinaryOpcode::Sub || dec->type_str != "i32" ||
      dec->lhs != phi->result || dec->rhs != "1" || dec->result != phi->incoming[1].value ||
      *exit.terminator.value != phi->result) {
    return std::nullopt;
  }

  const auto initial_imm = parse_i64(phi->incoming[0].value);
  if (!initial_imm.has_value() || *initial_imm < 0 ||
      *initial_imm > std::numeric_limits<std::uint16_t>::max()) {
    return std::nullopt;
  }

  return MinimalCountdownLoopSlice{*initial_imm, loop.label, body.label, exit.label};
}

std::optional<MinimalConditionalPhiJoinSlice> parse_minimal_conditional_phi_join_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto* function = find_function(module, "main");
  if (function == nullptr || function->is_declaration ||
      function->signature.linkage != "define" ||
      function->signature.return_type != "i32" ||
      !function->signature.params.empty() || function->signature.is_vararg ||
      function->blocks.size() != 4) {
    return std::nullopt;
  }

  const auto& entry = function->blocks[0];
  const auto& true_block = function->blocks[1];
  const auto& false_block = function->blocks[2];
  const auto& join_block = function->blocks[3];
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
      if (inst == nullptr || inst->type_str != "i32" ||
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

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm,
                                      global.align_bytes > 0 ? global.align_bytes : 4};
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

  return MinimalScalarGlobalStoreReloadSlice{
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

  return MinimalScalarGlobalLoadSlice{global.name, *init_imm,
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
  if (function.is_declaration || function.signature_text != "define i32 @main()\n" ||
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

  return MinimalExternScalarGlobalLoadSlice{global.name};
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
      ret == nullptr || !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto byte_offset = *element_index * 4;
  if (byte_offset < 0 || byte_offset > 4095) {
    return std::nullopt;
  }

  return MinimalExternGlobalArrayLoadSlice{global.name, byte_offset};
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
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) != array_suffix) {
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
      index1->to_type != "i64" || index1->operand != "1") {
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
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64" || expected_diff->operand != "1") {
    return std::nullopt;
  }

  const auto cmp_predicate = cmp->predicate.typed();
  if (cmp->is_float || cmp_predicate != LirCmpPredicate::Eq || cmp->type_str != "i64" ||
      cmp->lhs != diff->result || cmp->rhs != expected_diff->result) {
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

  return MinimalGlobalCharPointerDiffSlice{global.name, *global_size, 1};
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
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) != array_suffix) {
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
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result) {
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
      cmp->lhs != scaled_diff->result || cmp->rhs != expected_diff->result) {
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
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
    return std::nullopt;
  }

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalLoadSlice{
      global->name,
      *init_imm,
      global->align_bytes > 0 ? global->align_bytes : 4,
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

  return MinimalScalarGlobalStoreReloadSlice{
      global->name,
      *init_imm,
      *store_imm,
      global->align_bytes > 0 ? global->align_bytes : 4,
  };
}

std::optional<MinimalExternScalarGlobalLoadSlice> parse_minimal_extern_scalar_global_load_slice(
    const c4c::backend::BackendModule& module) {
  if (module.functions.size() != 1 || module.globals.size() != 1) {
    return std::nullopt;
  }

  const auto* global = find_global(module, module.globals.front().name);
  if (global == nullptr || !global->is_extern_decl || global->qualifier != "global " ||
      global->linkage != "external " || global->llvm_type != "i32") {
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

  const auto* load = std::get_if<c4c::backend::BackendLoadInst>(&block.insts.front());
  if (load == nullptr || load->type_str != "i32" ||
      load->address.base_symbol != global->name || load->address.byte_offset != 0 ||
      *block.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalExternScalarGlobalLoadSlice{global->name};
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
      !block.terminator.value.has_value() || block.terminator.type_str != "i32") {
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
  const auto call_operands =
      call == nullptr ? std::nullopt
                      : c4c::backend::parse_backend_direct_global_two_typed_call_operands(
                            *call, "add_pair", "i32", "i32");
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result || !call_operands.has_value()) {
    return std::nullopt;
  }

  const auto arg0_imm = parse_i64(call_operands->first);
  const auto arg1_imm = parse_i64(call_operands->second);
  if (!arg0_imm.has_value() || !arg1_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, "add_pair");
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

  return MinimalDirectCallTwoArgAddSlice{
      "add_pair",
      *arg0_imm,
      *arg1_imm,
  };
}

std::optional<MinimalDirectCallSlice> parse_minimal_direct_call_two_arg_folded_slice(
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
  const auto parsed_call =
      call == nullptr ? std::nullopt : c4c::backend::parse_backend_direct_global_typed_call(*call);
  if (call == nullptr || call->return_type != "i32" || call->result.empty() ||
      *main_block.terminator.value != call->result || !parsed_call.has_value() ||
      !c4c::backend::backend_typed_call_matches(
          parsed_call->typed_call, std::array<std::string_view, 2>{"i32", "i32"})) {
    return std::nullopt;
  }

  const auto arg0_imm = parse_i64(parsed_call->typed_call.args[0].operand);
  const auto arg1_imm = parse_i64(parsed_call->typed_call.args[1].operand);
  if (!arg0_imm.has_value() || !arg1_imm.has_value()) {
    return std::nullopt;
  }

  const auto* callee_fn = find_function(module, parsed_call->symbol_name);
  if (callee_fn == nullptr || callee_fn->is_declaration ||
      callee_fn->signature.linkage != "define" ||
      callee_fn->signature.return_type != "i32" || callee_fn->signature.is_vararg ||
      callee_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto callee_imm =
      fold_minimal_direct_call_two_arg_callee_return(*callee_fn, *arg0_imm, *arg1_imm);
  if (!callee_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalDirectCallSlice{std::string(parsed_call->symbol_name), *callee_imm};
}

std::optional<MinimalCallCrossingDirectCallSlice>
parse_minimal_call_crossing_direct_call_slice(
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
  if (!helper_add_imm.has_value()) {
    return std::nullopt;
  }

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
  if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
    return std::nullopt;
  }

  return MinimalCallCrossingDirectCallSlice{
      "add_one",
      *lhs_imm + *rhs_imm,
      *helper_add_imm,
      source_add->result,
      call->result,
  };
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

std::string emit_minimal_return_sub_imm_asm(const c4c::backend::BackendModule& module,
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
  const std::string symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, symbol, true);
  out << "  sub w0, wzr, #" << neg_imm << "\n"
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

std::string emit_minimal_call_crossing_direct_call_asm(
    const c4c::backend::BackendModule& module,
    const c4c::backend::RegAllocIntegrationResult& regalloc,
    const MinimalCallCrossingDirectCallSlice& slice) {
  if (slice.source_imm < 0 ||
      slice.source_imm > std::numeric_limits<std::uint16_t>::max()) {
    fail_unsupported("call-crossing source immediates outside the minimal mov-supported range");
  }
  if (slice.helper_add_imm < 0 || slice.helper_add_imm > 4095) {
    fail_unsupported("call-crossing helper add immediates outside the minimal add-supported range");
  }

  const auto* source_reg =
      c4c::backend::stack_layout::find_assigned_reg(regalloc, slice.source_value);
  if (source_reg == nullptr ||
      !c4c::backend::stack_layout::uses_callee_saved_reg(regalloc, *source_reg)) {
    fail_unsupported("shared call-crossing regalloc state for the minimal direct-call slice");
  }

  std::vector<c4c::backend::PhysReg> saved_regs;
  saved_regs.push_back(*source_reg);

  const std::int64_t frame_size = aligned_call_frame_size(saved_regs.size());
  std::ostringstream out;
  const std::string helper_symbol = asm_symbol_name(module, slice.callee_name);
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, helper_symbol, false);
  out << "  add w0, w0, #" << slice.helper_add_imm << "\n"
      << "  ret\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  sub sp, sp, #" << frame_size << "\n";
  for (std::size_t i = 0; i < saved_regs.size(); ++i) {
    out << "  str " << gp_reg_name(saved_regs[i], false) << ", [sp, #"
        << (i * 8) << "]\n";
  }
  out << "  str x30, [sp, #" << (saved_regs.size() * 8) << "]\n"
      << "  mov " << gp_reg_name(*source_reg, true) << ", #" << slice.source_imm << "\n"
      << "  mov w0, " << gp_reg_name(*source_reg, true) << "\n"
      << "  bl " << helper_symbol << "\n"
      << "  add w0, " << gp_reg_name(*source_reg, true) << ", w0\n"
      << "  ldr x30, [sp, #" << (saved_regs.size() * 8) << "]\n";
  for (std::size_t i = saved_regs.size(); i > 0; --i) {
    const auto& reg = saved_regs[i - 1];
    out << "  ldr " << gp_reg_name(reg, false) << ", [sp, #" << ((i - 1) * 8) << "]\n";
  }
  out << "  add sp, sp, #" << frame_size << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_conditional_return_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalConditionalReturnSlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  if (!in_mov_range(slice.lhs_imm) || !in_mov_range(slice.rhs_imm) ||
      !in_mov_range(slice.true_return_imm) || !in_mov_range(slice.false_return_imm)) {
    fail_unsupported("conditional-return immediates outside the minimal mov-supported range");
  }

  std::ostringstream out;
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const std::string main_symbol = asm_symbol_name(backend_module, "main");
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
  emit_function_prelude(out, backend_module, main_symbol, true);
  out << "  mov w8, #" << slice.lhs_imm << "\n"
      << "  cmp w8, #" << slice.rhs_imm << "\n"
      << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n"
      << "  mov w0, #" << slice.true_return_imm << "\n"
      << "  ret\n"
      << ".L" << slice.false_label << ":\n"
      << "  mov w0, #" << slice.false_return_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_countdown_loop_asm(const c4c::backend::BackendModule& module,
                                            const MinimalCountdownLoopSlice& slice) {
  std::ostringstream out;
  const std::string main_symbol = asm_symbol_name(module, "main");

  out << ".text\n";
  emit_function_prelude(out, module, main_symbol, true);
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

std::string emit_minimal_conditional_phi_join_asm(
    const c4c::backend::BackendModule& module,
    const MinimalConditionalPhiJoinSlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  if (!in_mov_range(slice.lhs_imm) || !in_mov_range(slice.rhs_imm) ||
      !in_mov_range(slice.true_value.base_imm) || !in_mov_range(slice.false_value.base_imm)) {
    fail_unsupported("conditional-phi-join immediates outside the minimal mov-supported range");
  }
  for (const auto& step : slice.true_value.steps) {
    if (!in_mov_range(step.imm)) {
      fail_unsupported(
          "conditional-phi-join predecessor arithmetic immediates outside the minimal add/sub-supported range");
    }
  }
  for (const auto& step : slice.false_value.steps) {
    if (!in_mov_range(step.imm)) {
      fail_unsupported(
          "conditional-phi-join predecessor arithmetic immediates outside the minimal add/sub-supported range");
    }
  }
  if (slice.join_add_imm.has_value() &&
      (*slice.join_add_imm < 0 ||
       *slice.join_add_imm > std::numeric_limits<std::uint16_t>::max())) {
    fail_unsupported("conditional-phi-join add immediates outside the minimal add-supported range");
  }

  std::ostringstream out;
  const std::string main_symbol = asm_symbol_name(module, "main");
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
          "conditional-phi-join predicates outside the current compare-and-branch slice");
  }

  out << ".text\n";
  emit_function_prelude(out, module, main_symbol, true);
  out << "  mov w8, #" << slice.lhs_imm << "\n"
      << "  cmp w8, #" << slice.rhs_imm << "\n"
      << "  " << fail_branch << " .L" << slice.false_label << "\n"
      << ".L" << slice.true_label << ":\n"
      << "  mov w0, #" << slice.true_value.base_imm << "\n";
  for (const auto& step : slice.true_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " w0, w0, #" << step.imm << "\n";
  }
  out << "  b .L" << slice.join_label << "\n"
      << ".L" << slice.false_label << ":\n"
      << "  mov w0, #" << slice.false_value.base_imm << "\n";
  for (const auto& step : slice.false_value.steps) {
    out << "  "
        << (step.opcode == c4c::backend::BackendBinaryOpcode::Sub ? "sub" : "add")
        << " w0, w0, #" << step.imm << "\n";
  }
  out
      << ".L" << slice.join_label << ":\n";
  if (slice.join_add_imm.has_value()) {
    out << "  add w0, w0, #" << *slice.join_add_imm << "\n";
  }
  out << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_load_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalScalarGlobalLoadSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max()) {
    fail_unsupported("scalar global initializers outside the minimal .long-supported range");
  }

  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");
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
  emit_function_prelude(out, backend_module, main_symbol, true);
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
    const c4c::codegen::lir::LirModule& module,
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

  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");
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
  emit_function_prelude(out, backend_module, main_symbol, true);
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
    const c4c::codegen::lir::LirModule& module,
    const MinimalExternScalarGlobalLoadSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  if (!is_darwin) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
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
    const c4c::codegen::lir::LirModule& module,
    const MinimalExternGlobalArrayLoadSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  if (!is_darwin) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
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

std::string emit_minimal_global_char_pointer_diff_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalGlobalCharPointerDiffSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  out << ".bss\n"
      << ".globl " << global_symbol << "\n";
  if (!is_darwin) {
    out << ".type " << global_symbol << ", %object\n";
  }
  out << ".p2align 0\n"
      << global_symbol << ":\n"
      << "  .zero " << slice.global_size << "\n";
  if (!is_darwin) {
    out << ".size " << global_symbol << ", " << slice.global_size << "\n";
  }

  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  add x8, x8, " << global_symbol << "@PAGEOFF\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  add x8, x8, :lo12:" << global_symbol << "\n";
  }
  out << "  add x9, x8, #" << slice.byte_offset << "\n"
      << "  sub x8, x9, x8\n"
      << "  cmp x8, #" << slice.byte_offset << "\n"
      << "  cset w0, eq\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_int_pointer_diff_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalGlobalIntPointerDiffSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string global_symbol =
      asm_symbol_name(backend_module, slice.global_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  out << ".bss\n"
      << ".globl " << global_symbol << "\n";
  if (!is_darwin) {
    out << ".type " << global_symbol << ", %object\n";
  }
  out << ".p2align 2\n"
      << global_symbol << ":\n"
      << "  .zero " << (slice.global_size * 4) << "\n";
  if (!is_darwin) {
    out << ".size " << global_symbol << ", " << (slice.global_size * 4) << "\n";
  }

  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  if (is_darwin) {
    out << "  adrp x8, " << global_symbol << "@PAGE\n"
        << "  add x8, x8, " << global_symbol << "@PAGEOFF\n";
  } else {
    out << "  adrp x8, " << global_symbol << "\n"
        << "  add x8, x8, :lo12:" << global_symbol << "\n";
  }
  out << "  add x9, x8, #" << slice.byte_offset << "\n"
      << "  sub x8, x9, x8\n"
      << "  lsr x8, x8, #" << slice.element_shift << "\n"
      << "  cmp x8, #1\n"
      << "  cset w0, eq\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_string_literal_char_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalStringLiteralCharSlice& slice) {
  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const bool is_darwin =
      backend_module.target_triple.find("apple-darwin") != std::string::npos;
  const std::string string_label = asm_private_data_label(module, slice.pool_name);
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  if (is_darwin) {
    out << ".section __TEXT,__cstring,cstring_literals\n";
  } else {
    out << ".section .rodata\n";
  }
  out << string_label << ":\n"
      << "  .asciz \"" << escape_asm_string(slice.raw_bytes) << "\"\n";
  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
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

std::string emit_minimal_local_array_asm(
    const c4c::codegen::lir::LirModule& module,
    const MinimalLocalArraySlice& slice) {
  const auto in_mov_range = [](std::int64_t imm) {
    return imm >= 0 && imm <= std::numeric_limits<std::uint16_t>::max();
  };
  if (!in_mov_range(slice.store0_imm) || !in_mov_range(slice.store1_imm)) {
    fail_unsupported("local-array store immediates outside the minimal mov-supported range");
  }

  c4c::backend::BackendModule backend_module;
  backend_module.target_triple = module.target_triple;
  const std::string main_symbol = asm_symbol_name(backend_module, "main");

  std::ostringstream out;
  out << ".text\n";
  emit_function_prelude(out, backend_module, main_symbol, true);
  out << "  sub sp, sp, #16\n"
      << "  add x8, sp, #8\n"
      << "  mov w9, #" << slice.store0_imm << "\n"
      << "  str w9, [x8]\n"
      << "  mov w9, #" << slice.store1_imm << "\n"
      << "  str w9, [x8, #4]\n"
      << "  ldr w9, [x8]\n"
      << "  ldr w10, [x8, #4]\n"
      << "  add w0, w9, w10\n"
      << "  add sp, sp, #16\n"
      << "  ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::backend::BackendModule& module,
                        const c4c::codegen::lir::LirModule* legacy_fallback) {
  try {
    if (const auto slice = parse_minimal_scalar_global_load_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_scalar_global_store_reload_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_extern_scalar_global_load_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_extern_scalar_global_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_extern_global_array_load_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_extern_global_array_load_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_char_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_char_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_global_int_pointer_diff_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_global_int_pointer_diff_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_string_literal_char_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_string_literal_char_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_return_slice(module);
        slice.has_value()) {
      c4c::codegen::lir::LirModule scaffold_module;
      scaffold_module.target_triple = module.target_triple;
      return emit_minimal_conditional_return_asm(scaffold_module, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(module);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(module);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(module);
        slice.has_value()) {
      if (legacy_fallback == nullptr) {
        fail_unsupported("shared call-crossing direct-call slices still require legacy LIR fallback");
      }
      const auto* main_fn = find_lir_function(*legacy_fallback, "main");
      if (main_fn == nullptr) {
        fail_unsupported("main function for shared call-crossing direct-call slice");
      }
      return emit_minimal_call_crossing_direct_call_asm(
          module, run_shared_aarch64_regalloc(*main_fn), *slice);
    }
    if (const auto imm = parse_minimal_return_imm(module); imm.has_value()) {
      return emit_minimal_return_imm_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(module); imm.has_value()) {
      return emit_minimal_return_imm_asm(module, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(module); imm.has_value()) {
      return emit_minimal_return_sub_imm_asm(module, *imm);
    }
    if (const auto slice = parse_minimal_direct_call_slice(module); slice.has_value()) {
      return emit_minimal_direct_call_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_add_imm_slice(module);
        slice.has_value()) {
      return emit_minimal_direct_call_add_imm_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_two_arg_add_slice(module);
        slice.has_value()) {
      return emit_minimal_direct_call_two_arg_add_asm(module, *slice);
    }
    if (const auto slice = parse_minimal_direct_call_two_arg_folded_slice(module);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(module, *slice);
    }
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  } catch (const std::invalid_argument&) {
  }

  if (legacy_fallback != nullptr) {
    return emit_module(*legacy_fallback);
  }
  return c4c::backend::print_backend_ir(module);
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  validate_module(module);
  if (const auto imm = parse_minimal_lir_return_imm(module); imm.has_value()) {
    c4c::backend::BackendModule scaffold_module;
    scaffold_module.target_triple = module.target_triple;
    return emit_minimal_return_imm_asm(scaffold_module, *imm);
  }
  if (const auto imm = parse_minimal_lir_return_sub_imm(module); imm.has_value()) {
    c4c::backend::BackendModule scaffold_module;
    scaffold_module.target_triple = module.target_triple;
    return emit_minimal_return_sub_imm_asm(scaffold_module, *imm);
  }
  if (const auto imm = parse_minimal_lir_local_pointer_return_imm(module);
      imm.has_value()) {
    c4c::backend::BackendModule scaffold_module;
    scaffold_module.target_triple = module.target_triple;
    return emit_minimal_return_imm_asm(scaffold_module, *imm);
  }
  if (const auto imm = parse_minimal_lir_single_scalar_countdown_imm(module);
      imm.has_value()) {
    c4c::backend::BackendModule scaffold_module;
    scaffold_module.target_triple = module.target_triple;
    return emit_minimal_return_imm_asm(scaffold_module, *imm);
  }

  if (const auto imm2 = try_constant_fold_single_block(module); imm2.has_value()) {
    c4c::backend::BackendModule scaffold_module;
    scaffold_module.target_triple = module.target_triple;
    return emit_minimal_return_imm_asm(scaffold_module, *imm2);
  }

  const auto prepared = prepare_module_for_fallback(module);
  validate_module(prepared);
  if (const auto slice = parse_minimal_local_array_slice(prepared);
      slice.has_value()) {
    return emit_minimal_local_array_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_conditional_return_slice(prepared);
      slice.has_value()) {
    return emit_minimal_conditional_return_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_load_slice(prepared);
      slice.has_value()) {
    return emit_minimal_scalar_global_load_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_scalar_global_store_reload_slice(prepared);
      slice.has_value()) {
    return emit_minimal_scalar_global_store_reload_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_global_int_pointer_roundtrip_slice(prepared);
      slice.has_value()) {
    return emit_minimal_scalar_global_load_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_extern_scalar_global_load_slice(prepared);
      slice.has_value()) {
    return emit_minimal_extern_scalar_global_load_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_extern_global_array_load_slice(prepared);
      slice.has_value()) {
    return emit_minimal_extern_global_array_load_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_global_char_pointer_diff_slice(prepared);
      slice.has_value()) {
    return emit_minimal_global_char_pointer_diff_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_global_int_pointer_diff_slice(prepared);
      slice.has_value()) {
    return emit_minimal_global_int_pointer_diff_asm(prepared, *slice);
  }
  if (const auto slice = parse_minimal_string_literal_char_slice(prepared);
      slice.has_value()) {
    return emit_minimal_string_literal_char_asm(prepared, *slice);
  }
  try {
    const auto adapted = c4c::backend::adapt_minimal_module(prepared);
    if (const auto slice = parse_minimal_call_crossing_direct_call_slice(adapted);
        slice.has_value()) {
      const auto* main_fn = find_lir_function(prepared, "main");
      if (main_fn == nullptr) {
        fail_unsupported("main function for shared call-crossing direct-call slice");
      }
      return emit_minimal_call_crossing_direct_call_asm(
          adapted, run_shared_aarch64_regalloc(*main_fn), *slice);
    }
    if (const auto imm = parse_minimal_return_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_add_imm(adapted); imm.has_value()) {
      return emit_minimal_return_imm_asm(adapted, *imm);
    }
    if (const auto imm = parse_minimal_return_sub_imm(adapted); imm.has_value()) {
      return emit_minimal_return_sub_imm_asm(adapted, *imm);
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
    if (const auto slice = parse_minimal_direct_call_two_arg_folded_slice(adapted);
        slice.has_value()) {
      return emit_minimal_direct_call_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_countdown_loop_slice(adapted);
        slice.has_value()) {
      return emit_minimal_countdown_loop_asm(adapted, *slice);
    }
    if (const auto slice = parse_minimal_conditional_phi_join_slice(adapted);
        slice.has_value()) {
      return emit_minimal_conditional_phi_join_asm(adapted, *slice);
    }
    return c4c::backend::render_module(adapted);
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return c4c::codegen::lir::print_llvm(prepared);
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(
      assembler::AssembleRequest{.asm_text = emit_module(module), .output_path = output_path});
}

}  // namespace c4c::backend::aarch64
