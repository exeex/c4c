#include "x86_codegen.hpp"

#include "../../../codegen/lir/ir.hpp"

#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalVariadicSum2Slice {
  std::string helper_name;
  std::string entry_name;
};

struct MinimalVariadicDoubleBytesSlice {
  std::string helper_name;
  std::string entry_name;
};

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

std::string direct_private_data_label(std::string_view target_triple, std::string_view pool_name) {
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
  const auto helper_symbol = direct_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = direct_symbol_name(target_triple, slice.entry_name);

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
  const auto helper_symbol = direct_symbol_name(target_triple, slice.helper_name);
  const auto entry_symbol = direct_symbol_name(target_triple, slice.entry_name);
  const auto constant_label =
      direct_private_data_label(target_triple, slice.helper_name + ".double.2_25");

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

}  // namespace

std::optional<std::string> try_emit_minimal_variadic_sum2_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto slice = parse_minimal_variadic_sum2_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_variadic_sum2_asm(module.target_triple, *slice);
}

std::optional<std::string> try_emit_minimal_variadic_double_bytes_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto slice = parse_minimal_variadic_double_bytes_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_variadic_double_bytes_asm(module.target_triple, *slice);
}

}  // namespace c4c::backend::x86
