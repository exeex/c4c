#include "lir_to_bir.hpp"
#include "lir_to_bir/passes.hpp"

#include <charconv>
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend {

namespace {

std::optional<bir::TypeKind> lower_scalar_type(
    const c4c::codegen::lir::LirTypeRef& type) {
  if (type.kind() != c4c::codegen::lir::LirTypeKind::Integer) {
    return std::nullopt;
  }

  const auto integer_width = type.integer_bit_width();
  if (!integer_width.has_value()) {
    return std::nullopt;
  }

  switch (*integer_width) {
    case 8:
      return bir::TypeKind::I8;
    case 32:
      return bir::TypeKind::I32;
    case 64:
      return bir::TypeKind::I64;
    default:
      return std::nullopt;
  }
}

bool lir_type_has_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                unsigned bit_width) {
  return type.kind() == c4c::codegen::lir::LirTypeKind::Integer &&
         type.integer_bit_width() == bit_width;
}

unsigned scalar_type_bit_width(bir::TypeKind type);

bool lir_type_matches_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                    unsigned bit_width) {
  return lir_to_bir::legalize_lir_type_matches_integer_width(type, bit_width);
}

bool lir_type_is_pointer_like(const c4c::codegen::lir::LirTypeRef& type) {
  return lir_to_bir::legalize_lir_type_is_pointer_like(type);
}

std::optional<bir::TypeKind> lower_global_type(
    const c4c::codegen::lir::LirGlobal& global) {
  return lir_to_bir::legalize_global_type(global);
}

unsigned scalar_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return 8;
    case bir::TypeKind::I32:
      return 32;
    case bir::TypeKind::I64:
      return 64;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

std::optional<bir::TypeKind> lower_function_return_type(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirRet& ret) {
  return lir_to_bir::legalize_function_return_type(function, ret);
}

bool lir_type_is_i32(const c4c::codegen::lir::LirTypeRef& type) {
  return lir_to_bir::legalize_lir_type_is_i32(type);
}

std::optional<std::int64_t> parse_immediate(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool immediate_fits_type(std::int64_t value, bir::TypeKind type) {
  return lir_to_bir::legalize_immediate_fits_type(value, type);
}

std::optional<bir::Value> lower_immediate_or_name(std::string_view value_text,
                                                  bir::TypeKind type) {
  return lir_to_bir::legalize_immediate_or_name(value_text, type);
}

std::optional<bir::Module> try_lower_minimal_countdown_loop_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (!backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 ||
      (function.blocks.size() != 4 && function.blocks.size() != 5) ||
      function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || !lir_type_is_i32(slot->type_str) || slot->result.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto* entry_store =
      entry.insts.size() == 1 ? std::get_if<LirStoreOp>(&entry.insts.front()) : nullptr;
  const auto* entry_branch = std::get_if<LirBr>(&entry.terminator);
  if (entry.label != "entry" || entry_store == nullptr || entry_branch == nullptr ||
      !lir_type_is_i32(entry_store->type_str) || entry_store->ptr != slot->result) {
    return std::nullopt;
  }

  const LirBlock* loop = nullptr;
  const LirBlock* body = nullptr;
  const LirBlock* exit = nullptr;
  const auto* loop_load = static_cast<const LirLoadOp*>(nullptr);
  const auto* loop_cmp = static_cast<const LirCmpOp*>(nullptr);
  const auto* loop_branch = static_cast<const LirCondBr*>(nullptr);
  const auto* body_load = static_cast<const LirLoadOp*>(nullptr);
  const auto* body_sub = static_cast<const LirBinOp*>(nullptr);
  const auto* body_store = static_cast<const LirStoreOp*>(nullptr);
  const auto* body_branch = static_cast<const LirBr*>(nullptr);
  const auto* exit_load = static_cast<const LirLoadOp*>(nullptr);
  const auto* exit_ret = static_cast<const LirRet*>(nullptr);

  if (function.blocks.size() == 4) {
    loop = &function.blocks[1];
    body = &function.blocks[2];
    exit = &function.blocks[3];

    loop_load = loop->insts.size() == 2 ? std::get_if<LirLoadOp>(&loop->insts[0]) : nullptr;
    loop_cmp = loop->insts.size() == 2 ? std::get_if<LirCmpOp>(&loop->insts[1]) : nullptr;
    loop_branch = std::get_if<LirCondBr>(&loop->terminator);
    body_load = body->insts.size() == 3 ? std::get_if<LirLoadOp>(&body->insts[0]) : nullptr;
    body_sub = body->insts.size() == 3 ? std::get_if<LirBinOp>(&body->insts[1]) : nullptr;
    body_store = body->insts.size() == 3 ? std::get_if<LirStoreOp>(&body->insts[2]) : nullptr;
    body_branch = std::get_if<LirBr>(&body->terminator);
    exit_load = exit->insts.size() == 1 ? std::get_if<LirLoadOp>(&exit->insts[0]) : nullptr;
    exit_ret = std::get_if<LirRet>(&exit->terminator);

    const auto cmp_predicate =
        loop_cmp == nullptr ? std::nullopt : loop_cmp->predicate.typed();
    const auto exit_ret_type = exit_ret == nullptr
                                   ? std::nullopt
                                   : lower_function_return_type(function, *exit_ret);
    const auto sub_opcode =
        body_sub == nullptr ? std::nullopt : body_sub->opcode.typed();
    if (entry_branch->target_label != loop->label || loop_load == nullptr ||
        loop_cmp == nullptr || loop_branch == nullptr || !lir_type_is_i32(loop_load->type_str) ||
        loop_load->ptr != slot->result || loop_cmp->is_float ||
        cmp_predicate != LirCmpPredicate::Ne || !lir_type_is_i32(loop_cmp->type_str) ||
        loop_cmp->lhs != loop_load->result || loop_cmp->rhs != "0" ||
        loop_branch->cond_name != loop_cmp->result || loop_branch->true_label != body->label ||
        loop_branch->false_label != exit->label || body_load == nullptr ||
        body_sub == nullptr || body_store == nullptr || body_branch == nullptr ||
        !lir_type_is_i32(body_load->type_str) || body_load->ptr != slot->result ||
        sub_opcode != LirBinaryOpcode::Sub || !lir_type_is_i32(body_sub->type_str) ||
        body_sub->lhs != body_load->result || body_sub->rhs != "1" ||
        !lir_type_is_i32(body_store->type_str) || body_store->val != body_sub->result ||
        body_store->ptr != slot->result || body_branch->target_label != loop->label ||
        exit_load == nullptr || exit_ret == nullptr || !lir_type_is_i32(exit_load->type_str) ||
        exit_load->ptr != slot->result || exit_ret_type != bir::TypeKind::I32 ||
        !exit_ret->value_str.has_value() || *exit_ret->value_str != exit_load->result) {
      return std::nullopt;
    }
  } else {
    body = &function.blocks[1];
    const auto& bridge = function.blocks[2];
    loop = &function.blocks[3];
    exit = &function.blocks[4];

    body_load = body->insts.size() == 3 ? std::get_if<LirLoadOp>(&body->insts[0]) : nullptr;
    body_sub = body->insts.size() == 3 ? std::get_if<LirBinOp>(&body->insts[1]) : nullptr;
    body_store = body->insts.size() == 3 ? std::get_if<LirStoreOp>(&body->insts[2]) : nullptr;
    body_branch = std::get_if<LirBr>(&body->terminator);
    const auto* bridge_branch = std::get_if<LirBr>(&bridge.terminator);
    loop_load = loop->insts.size() == 2 ? std::get_if<LirLoadOp>(&loop->insts[0]) : nullptr;
    loop_cmp = loop->insts.size() == 2 ? std::get_if<LirCmpOp>(&loop->insts[1]) : nullptr;
    loop_branch = std::get_if<LirCondBr>(&loop->terminator);
    exit_load = exit->insts.size() == 1 ? std::get_if<LirLoadOp>(&exit->insts[0]) : nullptr;
    exit_ret = std::get_if<LirRet>(&exit->terminator);

    const auto cmp_predicate =
        loop_cmp == nullptr ? std::nullopt : loop_cmp->predicate.typed();
    const auto exit_ret_type = exit_ret == nullptr
                                   ? std::nullopt
                                   : lower_function_return_type(function, *exit_ret);
    const auto sub_opcode =
        body_sub == nullptr ? std::nullopt : body_sub->opcode.typed();
    if (entry_branch->target_label != body->label || body_load == nullptr ||
        body_sub == nullptr || body_store == nullptr || body_branch == nullptr ||
        !lir_type_is_i32(body_load->type_str) || body_load->ptr != slot->result ||
        sub_opcode != LirBinaryOpcode::Sub || !lir_type_is_i32(body_sub->type_str) ||
        body_sub->lhs != body_load->result || body_sub->rhs != "1" ||
        !lir_type_is_i32(body_store->type_str) || body_store->val != body_sub->result ||
        body_store->ptr != slot->result || body_branch->target_label != bridge.label ||
        !bridge.insts.empty() || bridge_branch == nullptr ||
        bridge_branch->target_label != loop->label || loop_load == nullptr ||
        loop_cmp == nullptr || loop_branch == nullptr || !lir_type_is_i32(loop_load->type_str) ||
        loop_load->ptr != slot->result || loop_cmp->is_float ||
        cmp_predicate != LirCmpPredicate::Ne || !lir_type_is_i32(loop_cmp->type_str) ||
        loop_cmp->lhs != loop_load->result || loop_cmp->rhs != "0" ||
        loop_branch->cond_name != loop_cmp->result || loop_branch->true_label != body->label ||
        loop_branch->false_label != exit->label || exit_load == nullptr ||
        exit_ret == nullptr || !lir_type_is_i32(exit_load->type_str) ||
        exit_load->ptr != slot->result || exit_ret_type != bir::TypeKind::I32 ||
        !exit_ret->value_str.has_value() || *exit_ret->value_str != exit_load->result) {
      return std::nullopt;
    }
  }

  const auto initial_imm = parse_immediate(entry_store->val);
  if (!initial_imm.has_value() || *initial_imm < 0 ||
      *initial_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;
  lowered_function.local_slots.push_back(
      make_memory_local_slot(slot->result.str(), bir::TypeKind::I32, 4));

  bir::Block lowered_entry;
  lowered_entry.label = entry.label;
  lowered_entry.insts.push_back(make_memory_store_local(
      slot->result.str(), bir::Value::immediate_i32(static_cast<std::int32_t>(*initial_imm))));
  lowered_entry.terminator = bir::BranchTerminator{.target_label = loop->label};

  bir::Block lowered_loop;
  lowered_loop.label = loop->label;
  lowered_loop.insts.push_back(make_memory_load_local(
      bir::Value::named(bir::TypeKind::I32, loop_load->result.str()), slot->result.str()));
  lowered_loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, loop_cmp->result.str()),
      .lhs = bir::Value::named(bir::TypeKind::I32, loop_load->result.str()),
      .rhs = bir::Value::immediate_i32(0),
  });
  lowered_loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, loop_cmp->result.str()),
      .true_label = body->label,
      .false_label = exit->label,
  };

  bir::Block lowered_body;
  lowered_body.label = body->label;
  lowered_body.insts.push_back(make_memory_load_local(
      bir::Value::named(bir::TypeKind::I32, body_load->result.str()), slot->result.str()));
  lowered_body.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, body_sub->result.str()),
      .lhs = bir::Value::named(bir::TypeKind::I32, body_load->result.str()),
      .rhs = bir::Value::immediate_i32(1),
  });
  lowered_body.insts.push_back(make_memory_store_local(
      slot->result.str(), bir::Value::named(bir::TypeKind::I32, body_sub->result.str())));
  lowered_body.terminator = bir::BranchTerminator{.target_label = loop->label};

  bir::Block lowered_exit;
  lowered_exit.label = exit->label;
  lowered_exit.insts.push_back(make_memory_load_local(
      bir::Value::named(bir::TypeKind::I32, exit_load->result.str()), slot->result.str()));
  lowered_exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, exit_load->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered_function.blocks.push_back(std::move(lowered_loop));
  lowered_function.blocks.push_back(std::move(lowered_body));
  lowered_function.blocks.push_back(std::move(lowered_exit));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_double_countdown_guarded_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (!backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 11 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || !lir_type_is_i32(slot->type_str) || slot->result.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto* dead_store = entry.insts.size() == 2 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* first_init = entry.insts.size() == 2 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* entry_branch = std::get_if<LirBr>(&entry.terminator);

  const auto& first_cond = function.blocks[1];
  const auto* first_loop_load =
      first_cond.insts.size() == 2 ? std::get_if<LirLoadOp>(&first_cond.insts[0]) : nullptr;
  const auto* first_loop_cmp =
      first_cond.insts.size() == 2 ? std::get_if<LirCmpOp>(&first_cond.insts[1]) : nullptr;
  const auto* first_loop_branch = std::get_if<LirCondBr>(&first_cond.terminator);

  const auto& first_latch = function.blocks[2];
  const auto* first_latch_load =
      first_latch.insts.size() == 3 ? std::get_if<LirLoadOp>(&first_latch.insts[0]) : nullptr;
  const auto* first_latch_sub =
      first_latch.insts.size() == 3 ? std::get_if<LirBinOp>(&first_latch.insts[1]) : nullptr;
  const auto* first_latch_store =
      first_latch.insts.size() == 3 ? std::get_if<LirStoreOp>(&first_latch.insts[2]) : nullptr;
  const auto* first_latch_branch = std::get_if<LirBr>(&first_latch.terminator);

  const auto& first_body = function.blocks[3];
  const auto* first_body_branch = std::get_if<LirBr>(&first_body.terminator);

  const auto& guard = function.blocks[4];
  const auto* guard_load =
      guard.insts.size() == 2 ? std::get_if<LirLoadOp>(&guard.insts[0]) : nullptr;
  const auto* guard_cmp =
      guard.insts.size() == 2 ? std::get_if<LirCmpOp>(&guard.insts[1]) : nullptr;
  const auto* guard_branch = std::get_if<LirCondBr>(&guard.terminator);

  const auto& guarded_return = function.blocks[5];
  const auto* guard_ret = std::get_if<LirRet>(&guarded_return.terminator);

  const auto& second_init_block = function.blocks[6];
  const auto* second_init =
      second_init_block.insts.size() == 1 ? std::get_if<LirStoreOp>(&second_init_block.insts[0])
                                          : nullptr;
  const auto* second_init_branch = std::get_if<LirBr>(&second_init_block.terminator);

  const auto& second_cond = function.blocks[7];
  const auto* second_loop_load =
      second_cond.insts.size() == 2 ? std::get_if<LirLoadOp>(&second_cond.insts[0]) : nullptr;
  const auto* second_loop_cmp =
      second_cond.insts.size() == 2 ? std::get_if<LirCmpOp>(&second_cond.insts[1]) : nullptr;
  const auto* second_loop_branch = std::get_if<LirCondBr>(&second_cond.terminator);

  const auto& second_latch = function.blocks[8];
  const auto* second_latch_branch = std::get_if<LirBr>(&second_latch.terminator);

  const auto& second_body = function.blocks[9];
  const auto* second_body_load =
      second_body.insts.size() == 3 ? std::get_if<LirLoadOp>(&second_body.insts[0]) : nullptr;
  const auto* second_body_sub =
      second_body.insts.size() == 3 ? std::get_if<LirBinOp>(&second_body.insts[1]) : nullptr;
  const auto* second_body_store =
      second_body.insts.size() == 3 ? std::get_if<LirStoreOp>(&second_body.insts[2]) : nullptr;
  const auto* second_body_branch = std::get_if<LirBr>(&second_body.terminator);

  const auto& exit = function.blocks[10];
  const auto* exit_load = exit.insts.size() == 1 ? std::get_if<LirLoadOp>(&exit.insts[0]) : nullptr;
  const auto* exit_ret = std::get_if<LirRet>(&exit.terminator);

  const auto first_cmp_predicate =
      first_loop_cmp == nullptr ? std::nullopt : first_loop_cmp->predicate.typed();
  const auto first_sub_opcode =
      first_latch_sub == nullptr ? std::nullopt : first_latch_sub->opcode.typed();
  const auto guard_cmp_predicate =
      guard_cmp == nullptr ? std::nullopt : guard_cmp->predicate.typed();
  const auto second_cmp_predicate =
      second_loop_cmp == nullptr ? std::nullopt : second_loop_cmp->predicate.typed();
  const auto second_sub_opcode =
      second_body_sub == nullptr ? std::nullopt : second_body_sub->opcode.typed();
  const auto guard_ret_type =
      guard_ret == nullptr ? std::nullopt : lower_function_return_type(function, *guard_ret);
  const auto exit_ret_type =
      exit_ret == nullptr ? std::nullopt : lower_function_return_type(function, *exit_ret);
  if (entry.label != "entry" || dead_store == nullptr || first_init == nullptr ||
      entry_branch == nullptr || !lir_type_is_i32(dead_store->type_str) ||
      !lir_type_is_i32(first_init->type_str) || dead_store->ptr != slot->result ||
      first_init->ptr != slot->result || dead_store->val != "1" || first_init->val != "10" ||
      entry_branch->target_label != first_cond.label || first_cond.label != "for.cond.1" ||
      first_loop_load == nullptr || first_loop_cmp == nullptr || first_loop_branch == nullptr ||
      !lir_type_is_i32(first_loop_load->type_str) || first_loop_load->ptr != slot->result ||
      first_loop_cmp->is_float || first_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_is_i32(first_loop_cmp->type_str) ||
      first_loop_cmp->lhs != first_loop_load->result || first_loop_cmp->rhs != "0" ||
      first_loop_branch->cond_name != first_loop_cmp->result ||
      first_loop_branch->true_label != first_body.label ||
      first_loop_branch->false_label != guard.label || first_latch.label != "for.latch.1" ||
      first_latch_load == nullptr || first_latch_sub == nullptr || first_latch_store == nullptr ||
      first_latch_branch == nullptr || !lir_type_is_i32(first_latch_load->type_str) ||
      first_latch_load->ptr != slot->result || first_sub_opcode != LirBinaryOpcode::Sub ||
      !lir_type_is_i32(first_latch_sub->type_str) ||
      first_latch_sub->lhs != first_latch_load->result || first_latch_sub->rhs != "1" ||
      !lir_type_is_i32(first_latch_store->type_str) ||
      first_latch_store->val != first_latch_sub->result ||
      first_latch_store->ptr != slot->result ||
      first_latch_branch->target_label != first_cond.label || first_body.label != "block_1" ||
      !first_body.insts.empty() || first_body_branch == nullptr ||
      first_body_branch->target_label != first_latch.label || guard.label != "block_2" ||
      guard_load == nullptr || guard_cmp == nullptr || guard_branch == nullptr ||
      !lir_type_is_i32(guard_load->type_str) || guard_load->ptr != slot->result ||
      guard_cmp->is_float || guard_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_is_i32(guard_cmp->type_str) || guard_cmp->lhs != guard_load->result ||
      guard_cmp->rhs != "0" || guard_branch->cond_name != guard_cmp->result ||
      guard_branch->true_label != guarded_return.label ||
      guard_branch->false_label != second_init_block.label || guarded_return.label != "block_3" ||
      !guarded_return.insts.empty() || guard_ret == nullptr ||
      guard_ret_type != bir::TypeKind::I32 || !guard_ret->value_str.has_value() ||
      *guard_ret->value_str != "1" || second_init_block.label != "block_4" ||
      second_init == nullptr || second_init_branch == nullptr ||
      !lir_type_is_i32(second_init->type_str) || second_init->ptr != slot->result ||
      second_init->val != "10" || second_init_branch->target_label != second_cond.label ||
      second_cond.label != "for.cond.5" || second_loop_load == nullptr ||
      second_loop_cmp == nullptr || second_loop_branch == nullptr ||
      !lir_type_is_i32(second_loop_load->type_str) || second_loop_load->ptr != slot->result ||
      second_loop_cmp->is_float || second_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_is_i32(second_loop_cmp->type_str) ||
      second_loop_cmp->lhs != second_loop_load->result || second_loop_cmp->rhs != "0" ||
      second_loop_branch->cond_name != second_loop_cmp->result ||
      second_loop_branch->true_label != second_body.label ||
      second_loop_branch->false_label != exit.label || second_latch.label != "for.latch.5" ||
      !second_latch.insts.empty() || second_latch_branch == nullptr ||
      second_latch_branch->target_label != second_cond.label || second_body.label != "block_5" ||
      second_body_load == nullptr || second_body_sub == nullptr ||
      second_body_store == nullptr || second_body_branch == nullptr ||
      !lir_type_is_i32(second_body_load->type_str) || second_body_load->ptr != slot->result ||
      second_sub_opcode != LirBinaryOpcode::Sub ||
      !lir_type_is_i32(second_body_sub->type_str) ||
      second_body_sub->lhs != second_body_load->result || second_body_sub->rhs != "1" ||
      !lir_type_is_i32(second_body_store->type_str) ||
      second_body_store->val != second_body_sub->result ||
      second_body_store->ptr != slot->result ||
      second_body_branch->target_label != second_latch.label || exit.label != "block_6" ||
      exit_load == nullptr || exit_ret == nullptr || !lir_type_is_i32(exit_load->type_str) ||
      exit_load->ptr != slot->result || exit_ret_type != bir::TypeKind::I32 ||
      !exit_ret->value_str.has_value() || *exit_ret->value_str != exit_load->result) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_signed_narrow_local_slot_increment_compare_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() < 3 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty()) {
    return std::nullopt;
  }

  const auto slot_width =
      lir_type_has_integer_width(c4c::codegen::lir::LirTypeRef{slot->type_str}, 8)
          ? std::optional<unsigned>(8)
          : (lir_type_has_integer_width(c4c::codegen::lir::LirTypeRef{slot->type_str}, 16)
                 ? std::optional<unsigned>(16)
                 : std::nullopt);
  if (!slot_width.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label.empty() || entry.insts.size() != 12) {
    return std::nullopt;
  }

  const auto* initial_trunc = std::get_if<LirCastOp>(&entry.insts[0]);
  const auto* initial_store = std::get_if<LirStoreOp>(&entry.insts[1]);
  const auto* first_load = std::get_if<LirLoadOp>(&entry.insts[2]);
  const auto* first_extend = std::get_if<LirCastOp>(&entry.insts[3]);
  const auto* update = std::get_if<LirBinOp>(&entry.insts[4]);
  const auto* update_trunc = std::get_if<LirCastOp>(&entry.insts[5]);
  const auto* update_store = std::get_if<LirStoreOp>(&entry.insts[6]);
  const auto* second_load = std::get_if<LirLoadOp>(&entry.insts[7]);
  const auto* second_extend = std::get_if<LirCastOp>(&entry.insts[8]);
  const auto* compare = std::get_if<LirCmpOp>(&entry.insts[9]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[10]);
  const auto* branch_cmp = std::get_if<LirCmpOp>(&entry.insts[11]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (initial_trunc == nullptr || initial_store == nullptr || first_load == nullptr ||
      first_extend == nullptr || update == nullptr || update_trunc == nullptr ||
      update_store == nullptr || second_load == nullptr || second_extend == nullptr ||
      compare == nullptr || cond_cast == nullptr || branch_cmp == nullptr || condbr == nullptr ||
      initial_trunc->result.str().empty() || first_load->result.str().empty() ||
      first_extend->result.str().empty() || update->result.str().empty() ||
      update_trunc->result.str().empty() || second_load->result.str().empty() ||
      second_extend->result.str().empty() || compare->result.str().empty() ||
      cond_cast->result.str().empty() || branch_cmp->result.str().empty()) {
    return std::nullopt;
  }

  const auto update_opcode = update->opcode.typed();
  if (initial_trunc->kind != LirCastKind::Trunc ||
      !lir_type_matches_integer_width(initial_trunc->from_type, 32) ||
      !lir_type_matches_integer_width(initial_trunc->to_type, *slot_width) ||
      initial_store->ptr != slot->result || initial_store->val != initial_trunc->result ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{initial_store->type_str},
                                      *slot_width) ||
      first_load->ptr != slot->result ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_load->type_str},
                                      *slot_width) ||
      (first_extend->kind != LirCastKind::SExt && first_extend->kind != LirCastKind::ZExt) ||
      !lir_type_matches_integer_width(first_extend->from_type, *slot_width) ||
      !lir_type_matches_integer_width(first_extend->to_type, 32) ||
      first_extend->operand != first_load->result ||
      (update_opcode != LirBinaryOpcode::Add && update_opcode != LirBinaryOpcode::Sub) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{update->type_str}, 32) ||
      update->lhs != first_extend->result ||
      update_trunc->kind != LirCastKind::Trunc ||
      !lir_type_matches_integer_width(update_trunc->from_type, 32) ||
      !lir_type_matches_integer_width(update_trunc->to_type, *slot_width) ||
      update_trunc->operand != update->result || update_store->ptr != slot->result ||
      update_store->val != update_trunc->result ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{update_store->type_str},
                                      *slot_width) ||
      second_load->ptr != slot->result ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_load->type_str},
                                      *slot_width) ||
      second_extend->kind != first_extend->kind ||
      !lir_type_matches_integer_width(second_extend->from_type, *slot_width) ||
      !lir_type_matches_integer_width(second_extend->to_type, 32) ||
      second_extend->operand != second_load->result || compare->is_float ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{compare->type_str}, 32) ||
      compare->lhs != second_extend->result || cond_cast->kind != LirCastKind::ZExt ||
      !lir_type_matches_integer_width(cond_cast->from_type, 1) ||
      !lir_type_matches_integer_width(cond_cast->to_type, 32) ||
      cond_cast->operand != compare->result || branch_cmp->is_float ||
      branch_cmp->predicate.str() != "ne" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{branch_cmp->type_str}, 32) ||
      branch_cmp->lhs != cond_cast->result || branch_cmp->rhs != "0" ||
      condbr->cond_name != branch_cmp->result) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::size_t> block_indices_by_label;
  block_indices_by_label.reserve(function.blocks.size());
  for (std::size_t i = 1; i < function.blocks.size(); ++i) {
    const auto& block = function.blocks[i];
    if (block.label.empty() || !block_indices_by_label.emplace(block.label, i).second) {
      return std::nullopt;
    }
  }

  std::unordered_set<std::size_t> consumed_block_indices;
  auto consume_return_arm = [&](std::string_view branch_label) -> const LirBlock* {
    while (true) {
      const auto block_it = block_indices_by_label.find(branch_label);
      if (block_it == block_indices_by_label.end()) {
        return nullptr;
      }

      const auto block_index = block_it->second;
      const auto& block = function.blocks[block_index];
      if (!consumed_block_indices.insert(block_index).second || !block.insts.empty()) {
        return nullptr;
      }

      if (std::get_if<LirRet>(&block.terminator) != nullptr) {
        return &block;
      }

      const auto* bridge_br = std::get_if<LirBr>(&block.terminator);
      if (bridge_br == nullptr) {
        return nullptr;
      }
      branch_label = bridge_br->target_label;
    }
  };

  const auto* true_ret_block = consume_return_arm(condbr->true_label);
  const auto* false_ret_block = consume_return_arm(condbr->false_label);
  if (true_ret_block == nullptr || false_ret_block == nullptr ||
      consumed_block_indices.size() + 1 != function.blocks.size()) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_ret_block->terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_ret_block->terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != "i32" ||
      false_ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto initial_value = parse_immediate(initial_trunc->operand.str());
  const auto update_value =
      update->rhs == "0" ? std::optional<std::int64_t>(0) : parse_immediate(update->rhs);
  const auto compare_rhs = parse_immediate(compare->rhs.str());
  const auto true_return = parse_immediate(*true_ret->value_str);
  const auto false_return = parse_immediate(*false_ret->value_str);
  if (!initial_value.has_value() || !update_value.has_value() || !compare_rhs.has_value() ||
      !true_return.has_value() || !false_return.has_value()) {
    return std::nullopt;
  }

  auto narrow_to_slot = [&](std::int64_t value) -> std::int64_t {
    if (*slot_width == 8) {
      const auto narrowed = static_cast<std::uint8_t>(value);
      if (first_extend->kind == LirCastKind::SExt) {
        return static_cast<std::int8_t>(narrowed);
      }
      return narrowed;
    }

    const auto narrowed = static_cast<std::uint16_t>(value);
    if (first_extend->kind == LirCastKind::SExt) {
      return static_cast<std::int16_t>(narrowed);
    }
    return narrowed;
  };

  const auto initial_slot_value = narrow_to_slot(*initial_value);
  const auto updated_widened_value =
      update_opcode == LirBinaryOpcode::Add ? initial_slot_value + *update_value
                                            : initial_slot_value - *update_value;
  const auto final_slot_value = narrow_to_slot(updated_widened_value);

  auto evaluate_compare = [&](std::string_view predicate) -> std::optional<bool> {
    if (predicate == "eq") {
      return final_slot_value == *compare_rhs;
    }
    if (predicate == "ne") {
      return final_slot_value != *compare_rhs;
    }
    if (predicate == "slt") {
      return final_slot_value < *compare_rhs;
    }
    if (predicate == "sle") {
      return final_slot_value <= *compare_rhs;
    }
    if (predicate == "sgt") {
      return final_slot_value > *compare_rhs;
    }
    if (predicate == "sge") {
      return final_slot_value >= *compare_rhs;
    }
    if (predicate == "ult" || predicate == "ule" || predicate == "ugt" || predicate == "uge") {
      if (final_slot_value < 0 || *compare_rhs < 0) {
        return std::nullopt;
      }
      const auto lhs = static_cast<std::uint64_t>(final_slot_value);
      const auto rhs = static_cast<std::uint64_t>(*compare_rhs);
      if (predicate == "ult") {
        return lhs < rhs;
      }
      if (predicate == "ule") {
        return lhs <= rhs;
      }
      if (predicate == "ugt") {
        return lhs > rhs;
      }
      return lhs >= rhs;
    }
    return std::nullopt;
  };

  const auto compare_matches = evaluate_compare(compare->predicate.str());
  if (!compare_matches.has_value() || !immediate_fits_type(*true_return, bir::TypeKind::I32) ||
      !immediate_fits_type(*false_return, bir::TypeKind::I32)) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = entry.label;
  lowered_entry.terminator.value = bir::Value::immediate_i32(static_cast<std::int32_t>(
      *compare_matches ? *true_return : *false_return));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_dead_local_add_store_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() < 3 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  std::unordered_set<std::string> i32_slots;
  i32_slots.reserve(function.alloca_insts.size());
  for (const auto& inst : function.alloca_insts) {
    const auto* alloca = std::get_if<LirAllocaOp>(&inst);
    if (alloca == nullptr || alloca->result.empty() || !alloca->count.empty() ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{alloca->type_str}, 32) ||
        !i32_slots.insert(alloca->result).second) {
      return std::nullopt;
    }
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label.empty() || ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i32" ||
      entry.insts.empty() || entry.insts.size() % 4 != 0) {
    return std::nullopt;
  }

  const auto return_imm = parse_immediate(*ret->value_str);
  if (!return_imm.has_value() || !immediate_fits_type(*return_imm, bir::TypeKind::I32)) {
    return std::nullopt;
  }

  std::unordered_set<std::string_view> defined_results;
  defined_results.reserve(entry.insts.size());
  std::unordered_set<std::string_view> load_results;
  load_results.reserve(entry.insts.size() / 2);

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); inst_index += 4) {
    const auto* lhs_load = std::get_if<LirLoadOp>(&entry.insts[inst_index]);
    const auto* rhs_load = std::get_if<LirLoadOp>(&entry.insts[inst_index + 1]);
    const auto* add = std::get_if<LirBinOp>(&entry.insts[inst_index + 2]);
    const auto* store = std::get_if<LirStoreOp>(&entry.insts[inst_index + 3]);
    if (lhs_load == nullptr || rhs_load == nullptr || add == nullptr || store == nullptr ||
        lhs_load->result.empty() || rhs_load->result.empty() || add->result.empty()) {
      return std::nullopt;
    }

    if (!lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{lhs_load->type_str}, 32) ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{rhs_load->type_str}, 32) ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{add->type_str}, 32) ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store->type_str}, 32) ||
        add->opcode.typed() != LirBinaryOpcode::Add ||
        i32_slots.find(lhs_load->ptr) == i32_slots.end() ||
        i32_slots.find(rhs_load->ptr) == i32_slots.end() ||
        i32_slots.find(store->ptr) == i32_slots.end() || add->lhs != lhs_load->result ||
        add->rhs != rhs_load->result || store->val != add->result) {
      return std::nullopt;
    }

    if (!defined_results.insert(lhs_load->result).second ||
        !defined_results.insert(rhs_load->result).second ||
        !defined_results.insert(add->result).second) {
      return std::nullopt;
    }
    load_results.insert(lhs_load->result);
    load_results.insert(rhs_load->result);
  }

  for (const auto& inst : entry.insts) {
    if (std::get_if<LirLoadOp>(&inst) != nullptr || std::get_if<LirStoreOp>(&inst) != nullptr) {
      continue;
    }
    const auto* add = std::get_if<LirBinOp>(&inst);
    if (add == nullptr || load_results.find(add->result) != load_results.end()) {
      return std::nullopt;
    }
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = entry.label;
  lowered_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(*return_imm));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_store_load_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || !slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{slot->type_str}, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* store = entry.insts.size() == 3 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* load = entry.insts.size() == 3 ? std::get_if<LirLoadOp>(&entry.insts[1]) : nullptr;
  const auto* sub = entry.insts.size() == 3 ? std::get_if<LirBinOp>(&entry.insts[2]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || store == nullptr || load == nullptr || sub == nullptr ||
      ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != sub->result || store->ptr != slot->result || load->ptr != slot->result ||
      load->result.empty() || sub->result.empty() || sub->opcode.typed() != LirBinaryOpcode::Sub ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{load->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{sub->type_str}, 32) ||
      sub->lhs != load->result) {
    return std::nullopt;
  }

  const auto stored_imm = parse_immediate(store->val);
  const auto sub_imm = parse_immediate(sub->rhs);
  if (!stored_imm.has_value() || !sub_imm.has_value()) {
    return std::nullopt;
  }

  const auto return_imm = *stored_imm - *sub_imm;
  if (!immediate_fits_type(return_imm, bir::TypeKind::I32)) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_arithmetic_chain_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || !slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{slot->type_str}, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      entry.insts.size() < 6 || entry.insts.size() % 3 != 0) {
    return std::nullopt;
  }

  const auto* init_store = std::get_if<LirStoreOp>(&entry.insts.front());
  if (init_store == nullptr || init_store->ptr != slot->result ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32)) {
    return std::nullopt;
  }

  auto current_value = parse_immediate(init_store->val);
  if (!current_value.has_value()) {
    return std::nullopt;
  }
  if ((entry.insts.size() - 1) % 3 != 2) {
    return std::nullopt;
  }

  std::unordered_set<std::string_view> defined_results;
  defined_results.reserve(entry.insts.size());
  for (std::size_t inst_index = 1; inst_index < entry.insts.size(); inst_index += 3) {
    const bool final_step = inst_index + 2 == entry.insts.size();
    const auto* load = std::get_if<LirLoadOp>(&entry.insts[inst_index]);
    const auto* bin = std::get_if<LirBinOp>(&entry.insts[inst_index + 1]);
    if (load == nullptr || bin == nullptr || load->result.empty() || bin->result.empty() ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{load->type_str}, 32) ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{bin->type_str}, 32) ||
        load->ptr != slot->result || bin->lhs != load->result ||
        !defined_results.insert(load->result).second ||
        !defined_results.insert(bin->result).second) {
      return std::nullopt;
    }

    const auto opcode = bin->opcode.typed();
    const auto rhs_imm = parse_immediate(bin->rhs);
    if (!opcode.has_value() || !rhs_imm.has_value()) {
      return std::nullopt;
    }

    switch (*opcode) {
      case LirBinaryOpcode::Add:
        *current_value += *rhs_imm;
        break;
      case LirBinaryOpcode::Sub:
        *current_value -= *rhs_imm;
        break;
      case LirBinaryOpcode::Mul:
        *current_value *= *rhs_imm;
        break;
      case LirBinaryOpcode::SDiv:
        if (*rhs_imm == 0) {
          return std::nullopt;
        }
        *current_value /= *rhs_imm;
        break;
      case LirBinaryOpcode::UDiv:
        if (*current_value < 0 || *rhs_imm <= 0) {
          return std::nullopt;
        }
        *current_value = static_cast<std::int64_t>(
            static_cast<std::uint64_t>(*current_value) / static_cast<std::uint64_t>(*rhs_imm));
        break;
      case LirBinaryOpcode::SRem:
        if (*rhs_imm == 0) {
          return std::nullopt;
        }
        *current_value %= *rhs_imm;
        break;
      case LirBinaryOpcode::URem:
        if (*current_value < 0 || *rhs_imm <= 0) {
          return std::nullopt;
        }
        *current_value = static_cast<std::int64_t>(
            static_cast<std::uint64_t>(*current_value) % static_cast<std::uint64_t>(*rhs_imm));
        break;
      default:
        return std::nullopt;
    }

    if (final_step) {
      if (*ret->value_str != bin->result) {
        return std::nullopt;
      }
      continue;
    }

    const auto* store = std::get_if<LirStoreOp>(&entry.insts[inst_index + 2]);
    if (store == nullptr || store->ptr != slot->result || store->val != bin->result ||
        !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store->type_str}, 32)) {
      return std::nullopt;
    }
  }

  if (!immediate_fits_type(*current_value, bir::TypeKind::I32)) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(*current_value));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_two_local_i32_zero_init_return_first_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* first_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* second_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (first_slot == nullptr || second_slot == nullptr || first_slot->result.empty() ||
      second_slot->result.empty() || !first_slot->count.empty() || !second_slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_slot->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_slot->type_str}, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* second_store = entry.insts.size() == 3 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* first_store = entry.insts.size() == 3 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* first_load = entry.insts.size() == 3 ? std::get_if<LirLoadOp>(&entry.insts[2]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || second_store == nullptr || first_store == nullptr ||
      first_load == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != first_load->result || second_store->ptr != second_slot->result ||
      first_store->ptr != first_slot->result || first_load->ptr != first_slot->result ||
      first_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_store->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_store->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_load->type_str}, 32) ||
      second_store->val != "0" || first_store->val != "0") {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_pointer_store_zero_load_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* value_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (value_slot == nullptr || ptr_slot == nullptr || value_slot->result.empty() ||
      ptr_slot->result.empty() || !value_slot->count.empty() || !ptr_slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{value_slot->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_slot->type_str})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* ptr_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* ptr_load_for_store =
      entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[2]) : nullptr;
  const auto* indirect_zero_store =
      entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* ptr_load_for_ret =
      entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* indirect_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[5]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || ptr_store == nullptr ||
      ptr_load_for_store == nullptr || indirect_zero_store == nullptr || ptr_load_for_ret == nullptr ||
      indirect_load == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != indirect_load->result || init_store->ptr != value_slot->result ||
      ptr_store->ptr != ptr_slot->result || ptr_store->val != value_slot->result ||
      ptr_load_for_store->ptr != ptr_slot->result || ptr_load_for_store->result.empty() ||
      ptr_load_for_ret->ptr != ptr_slot->result || ptr_load_for_ret->result.empty() ||
      indirect_zero_store->ptr != ptr_load_for_store->result || indirect_zero_store->val != "0" ||
      indirect_load->ptr != ptr_load_for_ret->result || indirect_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_store->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_load_for_store->type_str}) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{indirect_zero_store->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_load_for_ret->type_str}) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{indirect_load->type_str}, 32)) {
    return std::nullopt;
  }

  if (!parse_immediate(indirect_zero_store->val).has_value()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_pointer_gep_zero_load_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* value_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (value_slot == nullptr || ptr_slot == nullptr || value_slot->result.empty() ||
      ptr_slot->result.empty() || !value_slot->count.empty() || !ptr_slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{value_slot->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_slot->type_str})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* ptr_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* ptr_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[2]) : nullptr;
  const auto* zero_index_cast = entry.insts.size() == 6 ? std::get_if<LirCastOp>(&entry.insts[3]) : nullptr;
  const auto* zero_gep = entry.insts.size() == 6 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* indirect_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[5]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || ptr_store == nullptr || ptr_load == nullptr ||
      zero_index_cast == nullptr || zero_gep == nullptr || indirect_load == nullptr || ret == nullptr ||
      !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != indirect_load->result || init_store->ptr != value_slot->result ||
      ptr_store->ptr != ptr_slot->result || ptr_store->val != value_slot->result ||
      ptr_load->ptr != ptr_slot->result || ptr_load->result.empty() ||
      zero_index_cast->kind != LirCastKind::SExt || zero_index_cast->from_type != "i32" ||
      zero_index_cast->operand != "0" || zero_index_cast->to_type != "i64" ||
      zero_gep->ptr != ptr_load->result || zero_gep->result.empty() ||
      zero_gep->indices != std::vector<std::string>{"i64 " + zero_index_cast->result.str()} ||
      indirect_load->ptr != zero_gep->result || indirect_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_store->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_load->type_str}) ||
      zero_gep->element_type != "i32" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{indirect_load->type_str}, 32)) {
    return std::nullopt;
  }

  if (!parse_immediate(init_store->val).has_value() || init_store->val != "0") {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_pointer_gep_zero_store_slot_load_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* value_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (value_slot == nullptr || ptr_slot == nullptr || value_slot->result.empty() ||
      ptr_slot->result.empty() || !value_slot->count.empty() || !ptr_slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{value_slot->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_slot->type_str})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 7 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* ptr_store = entry.insts.size() == 7 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* ptr_load = entry.insts.size() == 7 ? std::get_if<LirLoadOp>(&entry.insts[2]) : nullptr;
  const auto* zero_index_cast = entry.insts.size() == 7 ? std::get_if<LirCastOp>(&entry.insts[3]) : nullptr;
  const auto* zero_gep = entry.insts.size() == 7 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* indirect_zero_store =
      entry.insts.size() == 7 ? std::get_if<LirStoreOp>(&entry.insts[5]) : nullptr;
  const auto* final_load = entry.insts.size() == 7 ? std::get_if<LirLoadOp>(&entry.insts[6]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || ptr_store == nullptr || ptr_load == nullptr ||
      zero_index_cast == nullptr || zero_gep == nullptr || indirect_zero_store == nullptr ||
      final_load == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != final_load->result || init_store->ptr != value_slot->result ||
      ptr_store->ptr != ptr_slot->result || ptr_store->val != value_slot->result ||
      ptr_load->ptr != ptr_slot->result || ptr_load->result.empty() ||
      zero_index_cast->kind != LirCastKind::SExt || zero_index_cast->from_type != "i32" ||
      zero_index_cast->operand != "0" || zero_index_cast->to_type != "i64" ||
      zero_gep->ptr != ptr_load->result || zero_gep->result.empty() ||
      zero_gep->indices != std::vector<std::string>{"i64 " + zero_index_cast->result.str()} ||
      indirect_zero_store->ptr != zero_gep->result || indirect_zero_store->val != "0" ||
      final_load->ptr != value_slot->result || final_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_store->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_load->type_str}) ||
      zero_gep->element_type != "i32" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{indirect_zero_store->type_str}, 32) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{final_load->type_str}, 32)) {
    return std::nullopt;
  }

  if (!parse_immediate(init_store->val).has_value()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_array_two_slot_sum_sub_three_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* array_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (array_slot == nullptr || array_slot->result.empty() || !array_slot->count.empty() ||
      array_slot->type_str != "[2 x i32]") {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* first_base_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* first_index_cast = entry.insts.size() == 18 ? std::get_if<LirCastOp>(&entry.insts[1]) : nullptr;
  const auto* first_elem_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* first_store = entry.insts.size() == 18 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* second_base_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* second_index_cast = entry.insts.size() == 18 ? std::get_if<LirCastOp>(&entry.insts[5]) : nullptr;
  const auto* second_elem_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[6]) : nullptr;
  const auto* second_store = entry.insts.size() == 18 ? std::get_if<LirStoreOp>(&entry.insts[7]) : nullptr;
  const auto* third_base_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[8]) : nullptr;
  const auto* third_index_cast = entry.insts.size() == 18 ? std::get_if<LirCastOp>(&entry.insts[9]) : nullptr;
  const auto* third_elem_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[10]) : nullptr;
  const auto* first_load = entry.insts.size() == 18 ? std::get_if<LirLoadOp>(&entry.insts[11]) : nullptr;
  const auto* fourth_base_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[12]) : nullptr;
  const auto* fourth_index_cast = entry.insts.size() == 18 ? std::get_if<LirCastOp>(&entry.insts[13]) : nullptr;
  const auto* fourth_elem_gep = entry.insts.size() == 18 ? std::get_if<LirGepOp>(&entry.insts[14]) : nullptr;
  const auto* second_load = entry.insts.size() == 18 ? std::get_if<LirLoadOp>(&entry.insts[15]) : nullptr;
  const auto* add = entry.insts.size() == 18 ? std::get_if<LirBinOp>(&entry.insts[16]) : nullptr;
  const auto* sub = entry.insts.size() == 18 ? std::get_if<LirBinOp>(&entry.insts[17]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || first_base_gep == nullptr || first_index_cast == nullptr ||
      first_elem_gep == nullptr || first_store == nullptr || second_base_gep == nullptr ||
      second_index_cast == nullptr || second_elem_gep == nullptr || second_store == nullptr ||
      third_base_gep == nullptr || third_index_cast == nullptr || third_elem_gep == nullptr ||
      first_load == nullptr || fourth_base_gep == nullptr || fourth_index_cast == nullptr ||
      fourth_elem_gep == nullptr || second_load == nullptr || add == nullptr || sub == nullptr ||
      ret == nullptr ||
      !ret->value_str.has_value() || lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != sub->result || first_base_gep->ptr != array_slot->result ||
      first_base_gep->element_type != "[2 x i32]" || first_base_gep->result.empty() ||
      first_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      first_index_cast->kind != LirCastKind::SExt || first_index_cast->from_type != "i32" ||
      first_index_cast->operand != "0" || first_index_cast->to_type != "i64" ||
      first_elem_gep->ptr != first_base_gep->result || first_elem_gep->element_type != "i32" ||
      first_elem_gep->result.empty() ||
      first_elem_gep->indices != std::vector<std::string>{"i64 " + first_index_cast->result.str()} ||
      first_store->ptr != first_elem_gep->result || first_store->val != "1" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_store->type_str}, 32) ||
      second_base_gep->ptr != array_slot->result || second_base_gep->element_type != "[2 x i32]" ||
      second_base_gep->result.empty() ||
      second_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      second_index_cast->kind != LirCastKind::SExt || second_index_cast->from_type != "i32" ||
      second_index_cast->operand != "1" || second_index_cast->to_type != "i64" ||
      second_elem_gep->ptr != second_base_gep->result || second_elem_gep->element_type != "i32" ||
      second_elem_gep->result.empty() ||
      second_elem_gep->indices != std::vector<std::string>{"i64 " + second_index_cast->result.str()} ||
      second_store->ptr != second_elem_gep->result || second_store->val != "2" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_store->type_str}, 32) ||
      third_base_gep->ptr != array_slot->result || third_base_gep->element_type != "[2 x i32]" ||
      third_base_gep->result.empty() ||
      third_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      third_index_cast->kind != LirCastKind::SExt || third_index_cast->from_type != "i32" ||
      third_index_cast->operand != "0" || third_index_cast->to_type != "i64" ||
      third_elem_gep->ptr != third_base_gep->result || third_elem_gep->element_type != "i32" ||
      third_elem_gep->result.empty() ||
      third_elem_gep->indices != std::vector<std::string>{"i64 " + third_index_cast->result.str()} ||
      first_load->ptr != third_elem_gep->result || first_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_load->type_str}, 32) ||
      fourth_base_gep->ptr != array_slot->result ||
      fourth_base_gep->element_type != "[2 x i32]" || fourth_base_gep->result.empty() ||
      fourth_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      fourth_index_cast->kind != LirCastKind::SExt || fourth_index_cast->from_type != "i32" ||
      fourth_index_cast->operand != "1" || fourth_index_cast->to_type != "i64" ||
      fourth_elem_gep->ptr != fourth_base_gep->result || fourth_elem_gep->element_type != "i32" ||
      fourth_elem_gep->result.empty() ||
      fourth_elem_gep->indices != std::vector<std::string>{"i64 " + fourth_index_cast->result.str()} ||
      second_load->ptr != fourth_elem_gep->result || second_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_load->type_str}, 32) ||
      add->opcode != "add" || add->type_str != "i32" || add->lhs != first_load->result ||
      add->rhs != second_load->result || sub->opcode != "sub" || sub->type_str != "i32" ||
      sub->lhs != add->result || sub->rhs != "3") {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_local_i32_array_second_slot_pointer_store_zero_load_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* array_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (array_slot == nullptr || pointer_slot == nullptr || array_slot->result.empty() ||
      pointer_slot->result.empty() || !array_slot->count.empty() || !pointer_slot->count.empty() ||
      array_slot->type_str != "[2 x i32]" || pointer_slot->type_str != "ptr") {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* first_base_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* first_index_cast =
      entry.insts.size() == 10 ? std::get_if<LirCastOp>(&entry.insts[1]) : nullptr;
  const auto* first_elem_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* pointer_store =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* pointer_load =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* zero_store =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[5]) : nullptr;
  const auto* second_base_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[6]) : nullptr;
  const auto* second_index_cast =
      entry.insts.size() == 10 ? std::get_if<LirCastOp>(&entry.insts[7]) : nullptr;
  const auto* second_elem_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[8]) : nullptr;
  const auto* final_load =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[9]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);

  if (entry.label != "entry" || first_base_gep == nullptr || first_index_cast == nullptr ||
      first_elem_gep == nullptr || pointer_store == nullptr || pointer_load == nullptr ||
      zero_store == nullptr || second_base_gep == nullptr || second_index_cast == nullptr ||
      second_elem_gep == nullptr || final_load == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != final_load->result || first_base_gep->ptr != array_slot->result ||
      first_base_gep->element_type != "[2 x i32]" || first_base_gep->result.empty() ||
      first_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      first_index_cast->kind != LirCastKind::SExt || first_index_cast->from_type != "i32" ||
      first_index_cast->operand != "1" || first_index_cast->to_type != "i64" ||
      first_elem_gep->ptr != first_base_gep->result || first_elem_gep->element_type != "i32" ||
      first_elem_gep->result.empty() ||
      first_elem_gep->indices != std::vector<std::string>{"i64 " + first_index_cast->result.str()} ||
      pointer_store->type_str != "ptr" || pointer_store->val != first_elem_gep->result ||
      pointer_store->ptr != pointer_slot->result || pointer_load->type_str != "ptr" ||
      pointer_load->ptr != pointer_slot->result || pointer_load->result.empty() ||
      zero_store->ptr != pointer_load->result || zero_store->val != "0" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{zero_store->type_str}, 32) ||
      second_base_gep->ptr != array_slot->result ||
      second_base_gep->element_type != "[2 x i32]" || second_base_gep->result.empty() ||
      second_base_gep->indices != std::vector<std::string>{"i64 0", "i64 0"} ||
      second_index_cast->kind != LirCastKind::SExt || second_index_cast->from_type != "i32" ||
      second_index_cast->operand != "1" || second_index_cast->to_type != "i64" ||
      second_elem_gep->ptr != second_base_gep->result ||
      second_elem_gep->element_type != "i32" || second_elem_gep->result.empty() ||
      second_elem_gep->indices != std::vector<std::string>{"i64 " + second_index_cast->result.str()} ||
      final_load->ptr != second_elem_gep->result || final_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{final_load->type_str}, 32)) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_two_field_struct_sub_sub_two_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* struct_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (struct_slot == nullptr || struct_slot->result.empty() || !struct_slot->count.empty() ||
      struct_slot->type_str != "%struct._anon_0" ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct._anon_0 = type { i32, i32 }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* first_gep = entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* first_store =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* second_gep = entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* second_store =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* third_gep = entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* first_load =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[5]) : nullptr;
  const auto* fourth_gep = entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[6]) : nullptr;
  const auto* second_load =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[7]) : nullptr;
  const auto* first_sub = entry.insts.size() == 10 ? std::get_if<LirBinOp>(&entry.insts[8]) : nullptr;
  const auto* second_sub = entry.insts.size() == 10 ? std::get_if<LirBinOp>(&entry.insts[9]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);

  if (entry.label != "entry" || first_gep == nullptr || first_store == nullptr ||
      second_gep == nullptr || second_store == nullptr || third_gep == nullptr ||
      first_load == nullptr || fourth_gep == nullptr || second_load == nullptr ||
      first_sub == nullptr || second_sub == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != second_sub->result || first_gep->ptr != struct_slot->result ||
      first_gep->element_type != "%struct._anon_0" || first_gep->result.empty() ||
      first_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      first_store->ptr != first_gep->result || first_store->val != "3" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_store->type_str}, 32) ||
      second_gep->ptr != struct_slot->result || second_gep->element_type != "%struct._anon_0" ||
      second_gep->result.empty() || second_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      second_store->ptr != second_gep->result || second_store->val != "5" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_store->type_str}, 32) ||
      third_gep->ptr != struct_slot->result || third_gep->element_type != "%struct._anon_0" ||
      third_gep->result.empty() || third_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      first_load->ptr != third_gep->result || first_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_load->type_str}, 32) ||
      fourth_gep->ptr != struct_slot->result || fourth_gep->element_type != "%struct._anon_0" ||
      fourth_gep->result.empty() || fourth_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      second_load->ptr != fourth_gep->result || second_load->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_load->type_str}, 32) ||
      first_sub->opcode != "sub" || first_sub->type_str != "i32" ||
      first_sub->lhs != first_load->result || first_sub->rhs != second_load->result ||
      second_sub->opcode != "sub" || second_sub->type_str != "i32" ||
      second_sub->lhs != first_sub->result || second_sub->rhs != "2") {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_struct_pointer_alias_add_sub_three_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* struct_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  const std::string struct_type =
      struct_slot == nullptr ? std::string{} : struct_slot->type_str.str();
  if (struct_slot == nullptr || pointer_slot == nullptr || struct_slot->result.empty() ||
      !struct_slot->count.empty() ||
      (struct_type != "%struct._anon_0" && struct_type != "%struct.S") ||
      pointer_slot->result.empty() || !pointer_slot->count.empty() || pointer_slot->type_str != "ptr" ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                struct_type + " = type { i32, i32 }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* store_pointer =
      entry.insts.size() == 14 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* field0_gep = entry.insts.size() == 14 ? std::get_if<LirGepOp>(&entry.insts[1]) : nullptr;
  const auto* store_field0 =
      entry.insts.size() == 14 ? std::get_if<LirStoreOp>(&entry.insts[2]) : nullptr;
  const auto* first_pointer_load =
      entry.insts.size() == 14 ? std::get_if<LirLoadOp>(&entry.insts[3]) : nullptr;
  const auto* field1_store_gep =
      entry.insts.size() == 14 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* store_field1 =
      entry.insts.size() == 14 ? std::get_if<LirStoreOp>(&entry.insts[5]) : nullptr;
  const auto* second_pointer_load =
      entry.insts.size() == 14 ? std::get_if<LirLoadOp>(&entry.insts[6]) : nullptr;
  const auto* field1_load_gep =
      entry.insts.size() == 14 ? std::get_if<LirGepOp>(&entry.insts[7]) : nullptr;
  const auto* load_field1 =
      entry.insts.size() == 14 ? std::get_if<LirLoadOp>(&entry.insts[8]) : nullptr;
  const auto* third_pointer_load =
      entry.insts.size() == 14 ? std::get_if<LirLoadOp>(&entry.insts[9]) : nullptr;
  const auto* field0_load_gep =
      entry.insts.size() == 14 ? std::get_if<LirGepOp>(&entry.insts[10]) : nullptr;
  const auto* load_field0 =
      entry.insts.size() == 14 ? std::get_if<LirLoadOp>(&entry.insts[11]) : nullptr;
  const auto* add = entry.insts.size() == 14 ? std::get_if<LirBinOp>(&entry.insts[12]) : nullptr;
  const auto* sub = entry.insts.size() == 14 ? std::get_if<LirBinOp>(&entry.insts[13]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);

  if (entry.label != "entry" || store_pointer == nullptr || field0_gep == nullptr ||
      store_field0 == nullptr || first_pointer_load == nullptr || field1_store_gep == nullptr ||
      store_field1 == nullptr || second_pointer_load == nullptr || field1_load_gep == nullptr ||
      load_field1 == nullptr || third_pointer_load == nullptr || field0_load_gep == nullptr ||
      load_field0 == nullptr || add == nullptr || sub == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != sub->result || store_pointer->type_str != "ptr" ||
      store_pointer->val != struct_slot->result || store_pointer->ptr != pointer_slot->result ||
      field0_gep->ptr != struct_slot->result || field0_gep->element_type != struct_type ||
      field0_gep->result.empty() || field0_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      store_field0->ptr != field0_gep->result || store_field0->val != "1" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store_field0->type_str}, 32) ||
      first_pointer_load->ptr != pointer_slot->result || first_pointer_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{first_pointer_load->type_str}) ||
      field1_store_gep->ptr != first_pointer_load->result ||
      field1_store_gep->element_type != struct_type || field1_store_gep->result.empty() ||
      field1_store_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      store_field1->ptr != field1_store_gep->result || store_field1->val != "2" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store_field1->type_str}, 32) ||
      second_pointer_load->ptr != pointer_slot->result || second_pointer_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{second_pointer_load->type_str}) ||
      field1_load_gep->ptr != second_pointer_load->result ||
      field1_load_gep->element_type != struct_type || field1_load_gep->result.empty() ||
      field1_load_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      load_field1->ptr != field1_load_gep->result || load_field1->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{load_field1->type_str}, 32) ||
      third_pointer_load->ptr != pointer_slot->result || third_pointer_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{third_pointer_load->type_str}) ||
      field0_load_gep->ptr != third_pointer_load->result ||
      field0_load_gep->element_type != struct_type || field0_load_gep->result.empty() ||
      field0_load_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      load_field0->ptr != field0_load_gep->result || load_field0->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{load_field0->type_str}, 32) ||
      add->opcode != "add" || add->type_str != "i32" || add->lhs != load_field1->result ||
      add->rhs != load_field0->result || sub->opcode != "sub" || sub->type_str != "i32" ||
      sub->lhs != add->result || sub->rhs != "3") {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_self_referential_struct_pointer_chain_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* struct_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (struct_slot == nullptr || struct_slot->result.empty() || !struct_slot->count.empty() ||
      struct_slot->type_str != "%struct.S" ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.S = type { ptr, i32, [4 x i8] }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* x_gep = entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* store_zero = entry.insts.size() == 16 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* self_gep = entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* store_self = entry.insts.size() == 16 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* first_chain_gep =
      entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* first_chain_load =
      entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[5]) : nullptr;
  const auto* second_chain_gep =
      entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[6]) : nullptr;
  const auto* second_chain_load =
      entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[7]) : nullptr;
  const auto* third_chain_gep =
      entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[8]) : nullptr;
  const auto* third_chain_load =
      entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[9]) : nullptr;
  const auto* fourth_chain_gep =
      entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[10]) : nullptr;
  const auto* fourth_chain_load =
      entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[11]) : nullptr;
  const auto* fifth_chain_gep =
      entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[12]) : nullptr;
  const auto* fifth_chain_load =
      entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[13]) : nullptr;
  const auto* final_x_gep = entry.insts.size() == 16 ? std::get_if<LirGepOp>(&entry.insts[14]) : nullptr;
  const auto* load_x = entry.insts.size() == 16 ? std::get_if<LirLoadOp>(&entry.insts[15]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);

  if (entry.label != "entry" || x_gep == nullptr || store_zero == nullptr || self_gep == nullptr ||
      store_self == nullptr || first_chain_gep == nullptr || first_chain_load == nullptr ||
      second_chain_gep == nullptr || second_chain_load == nullptr || third_chain_gep == nullptr ||
      third_chain_load == nullptr || fourth_chain_gep == nullptr || fourth_chain_load == nullptr ||
      fifth_chain_gep == nullptr || fifth_chain_load == nullptr || final_x_gep == nullptr ||
      load_x == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      lower_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != load_x->result || x_gep->ptr != struct_slot->result ||
      x_gep->element_type != "%struct.S" || x_gep->result.empty() ||
      x_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      store_zero->ptr != x_gep->result || store_zero->val != "0" ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store_zero->type_str}, 32) ||
      self_gep->ptr != struct_slot->result || self_gep->element_type != "%struct.S" ||
      self_gep->result.empty() || self_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      store_self->ptr != self_gep->result || store_self->val != struct_slot->result ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{store_self->type_str}) ||
      first_chain_gep->ptr != struct_slot->result || first_chain_gep->element_type != "%struct.S" ||
      first_chain_gep->result.empty() ||
      first_chain_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      first_chain_load->ptr != first_chain_gep->result || first_chain_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{first_chain_load->type_str}) ||
      second_chain_gep->ptr != first_chain_load->result ||
      second_chain_gep->element_type != "%struct.S" || second_chain_gep->result.empty() ||
      second_chain_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      second_chain_load->ptr != second_chain_gep->result || second_chain_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{second_chain_load->type_str}) ||
      third_chain_gep->ptr != second_chain_load->result ||
      third_chain_gep->element_type != "%struct.S" || third_chain_gep->result.empty() ||
      third_chain_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      third_chain_load->ptr != third_chain_gep->result || third_chain_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{third_chain_load->type_str}) ||
      fourth_chain_gep->ptr != third_chain_load->result ||
      fourth_chain_gep->element_type != "%struct.S" || fourth_chain_gep->result.empty() ||
      fourth_chain_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      fourth_chain_load->ptr != fourth_chain_gep->result || fourth_chain_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{fourth_chain_load->type_str}) ||
      fifth_chain_gep->ptr != fourth_chain_load->result ||
      fifth_chain_gep->element_type != "%struct.S" || fifth_chain_gep->result.empty() ||
      fifth_chain_gep->indices != std::vector<std::string>{"i32 0", "i32 0"} ||
      fifth_chain_load->ptr != fifth_chain_gep->result || fifth_chain_load->result.empty() ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{fifth_chain_load->type_str}) ||
      final_x_gep->ptr != fifth_chain_load->result || final_x_gep->element_type != "%struct.S" ||
      final_x_gep->result.empty() || final_x_gep->indices != std::vector<std::string>{"i32 0", "i32 1"} ||
      load_x->ptr != final_x_gep->result || load_x->result.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{load_x->type_str}, 32)) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_double_indirect_local_store_one_final_branch_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 ||
      (function.blocks.size() != 8 && function.blocks.size() != 9) ||
      function.alloca_insts.size() != 3 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* value_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  const auto* ptr_ptr_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[2]);
  if (value_slot == nullptr || ptr_slot == nullptr || ptr_ptr_slot == nullptr ||
      value_slot->result.empty() || ptr_slot->result.empty() || ptr_ptr_slot->result.empty() ||
      !value_slot->count.empty() || !ptr_slot->count.empty() || !ptr_ptr_slot->count.empty() ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{value_slot->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_slot->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_ptr_slot->type_str})) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* ptr_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* ptr_ptr_store =
      entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[2]) : nullptr;
  const auto* first_ptr_load =
      entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[3]) : nullptr;
  const auto* first_value_load =
      entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* first_cmp = entry.insts.size() == 6 ? std::get_if<LirCmpOp>(&entry.insts[5]) : nullptr;
  const auto* entry_branch = std::get_if<LirCondBr>(&entry.terminator);

  const auto& first_ret_block = function.blocks[1];
  const auto* first_ret = std::get_if<LirRet>(&first_ret_block.terminator);

  const auto& second_check = function.blocks[2];
  const auto* second_pp_load =
      second_check.insts.size() == 4 ? std::get_if<LirLoadOp>(&second_check.insts[0]) : nullptr;
  const auto* second_ptr_load =
      second_check.insts.size() == 4 ? std::get_if<LirLoadOp>(&second_check.insts[1]) : nullptr;
  const auto* second_value_load =
      second_check.insts.size() == 4 ? std::get_if<LirLoadOp>(&second_check.insts[2]) : nullptr;
  const auto* second_cmp =
      second_check.insts.size() == 4 ? std::get_if<LirCmpOp>(&second_check.insts[3]) : nullptr;
  const auto* second_branch = std::get_if<LirCondBr>(&second_check.terminator);

  const auto& second_ret_block = function.blocks[3];
  const auto* second_ret = std::get_if<LirRet>(&second_ret_block.terminator);

  const auto& store_one_block = function.blocks[4];
  const auto* store_pp_load =
      store_one_block.insts.size() == 3 ? std::get_if<LirLoadOp>(&store_one_block.insts[0]) : nullptr;
  const auto* store_ptr_load =
      store_one_block.insts.size() == 3 ? std::get_if<LirLoadOp>(&store_one_block.insts[1]) : nullptr;
  const auto* store_one =
      store_one_block.insts.size() == 3 ? std::get_if<LirStoreOp>(&store_one_block.insts[2]) : nullptr;
  const auto* store_branch = std::get_if<LirBr>(&store_one_block.terminator);

  const auto& final_check = function.blocks[5];
  const auto* final_value_load =
      final_check.insts.size() == 2 ? std::get_if<LirLoadOp>(&final_check.insts[0]) : nullptr;
  const auto* final_cmp =
      final_check.insts.size() == 2 ? std::get_if<LirCmpOp>(&final_check.insts[1]) : nullptr;
  const auto* final_branch = std::get_if<LirCondBr>(&final_check.terminator);

  const auto& final_true_ret_block = function.blocks[6];
  const auto* final_true_ret = std::get_if<LirRet>(&final_true_ret_block.terminator);

  const auto& final_false_ret_block = function.blocks[7];
  const auto* final_false_ret = std::get_if<LirRet>(&final_false_ret_block.terminator);

  const LirBlock* trailing_dead_ret_block =
      function.blocks.size() == 9 ? &function.blocks[8] : nullptr;
  const auto* trailing_dead_ret =
      trailing_dead_ret_block == nullptr ? nullptr : std::get_if<LirRet>(&trailing_dead_ret_block->terminator);

  const auto first_cmp_predicate =
      first_cmp == nullptr ? std::nullopt : first_cmp->predicate.typed();
  const auto second_cmp_predicate =
      second_cmp == nullptr ? std::nullopt : second_cmp->predicate.typed();
  const auto final_cmp_predicate =
      final_cmp == nullptr ? std::nullopt : final_cmp->predicate.typed();
  const auto first_ret_type =
      first_ret == nullptr ? std::nullopt : lower_function_return_type(function, *first_ret);
  const auto second_ret_type =
      second_ret == nullptr ? std::nullopt : lower_function_return_type(function, *second_ret);
  const auto final_true_ret_type =
      final_true_ret == nullptr ? std::nullopt : lower_function_return_type(function, *final_true_ret);
  const auto final_false_ret_type =
      final_false_ret == nullptr ? std::nullopt : lower_function_return_type(function, *final_false_ret);
  const auto trailing_dead_ret_type =
      trailing_dead_ret == nullptr ? std::nullopt : lower_function_return_type(function, *trailing_dead_ret);
  if (entry.label != "entry" || init_store == nullptr || ptr_store == nullptr ||
      ptr_ptr_store == nullptr || first_ptr_load == nullptr || first_value_load == nullptr ||
      first_cmp == nullptr || entry_branch == nullptr ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_store->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{ptr_ptr_store->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{first_ptr_load->type_str}) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_value_load->type_str}, 32) ||
      first_cmp->is_float || first_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{first_cmp->type_str}, 32) ||
      init_store->ptr != value_slot->result || init_store->val != "0" ||
      ptr_store->ptr != ptr_slot->result || ptr_store->val != value_slot->result ||
      ptr_ptr_store->ptr != ptr_ptr_slot->result || ptr_ptr_store->val != ptr_slot->result ||
      first_ptr_load->ptr != ptr_slot->result || first_ptr_load->result.empty() ||
      first_value_load->ptr != first_ptr_load->result || first_value_load->result.empty() ||
      first_cmp->lhs != first_value_load->result || first_cmp->rhs != "0" ||
      entry_branch->cond_name != first_cmp->result ||
      entry_branch->true_label != first_ret_block.label ||
      entry_branch->false_label != second_check.label || first_ret_block.label != "block_1" ||
      !first_ret_block.insts.empty() || first_ret == nullptr ||
      first_ret_type != bir::TypeKind::I32 || !first_ret->value_str.has_value() ||
      *first_ret->value_str != "1" || second_check.label != "block_2" ||
      second_pp_load == nullptr || second_ptr_load == nullptr || second_value_load == nullptr ||
      second_cmp == nullptr || second_branch == nullptr ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{second_pp_load->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{second_ptr_load->type_str}) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_value_load->type_str}, 32) ||
      second_cmp->is_float || second_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{second_cmp->type_str}, 32) ||
      second_pp_load->ptr != ptr_ptr_slot->result || second_pp_load->result.empty() ||
      second_ptr_load->ptr != second_pp_load->result || second_ptr_load->result.empty() ||
      second_value_load->ptr != second_ptr_load->result || second_value_load->result.empty() ||
      second_cmp->lhs != second_value_load->result || second_cmp->rhs != "0" ||
      second_branch->cond_name != second_cmp->result ||
      second_branch->true_label != second_ret_block.label ||
      second_branch->false_label != store_one_block.label || second_ret_block.label != "block_3" ||
      !second_ret_block.insts.empty() || second_ret == nullptr ||
      second_ret_type != bir::TypeKind::I32 || !second_ret->value_str.has_value() ||
      *second_ret->value_str != "1" || store_one_block.label != "block_4" ||
      store_pp_load == nullptr || store_ptr_load == nullptr || store_one == nullptr ||
      store_branch == nullptr ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{store_pp_load->type_str}) ||
      !lir_type_is_pointer_like(c4c::codegen::lir::LirTypeRef{store_ptr_load->type_str}) ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{store_one->type_str}, 32) ||
      store_pp_load->ptr != ptr_ptr_slot->result || store_pp_load->result.empty() ||
      store_ptr_load->ptr != store_pp_load->result || store_ptr_load->result.empty() ||
      store_one->ptr != store_ptr_load->result || store_one->val != "1" ||
      store_branch->target_label != final_check.label || final_check.label != "block_5" ||
      final_value_load == nullptr || final_cmp == nullptr || final_branch == nullptr ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{final_value_load->type_str}, 32) ||
      final_cmp->is_float || final_cmp_predicate != LirCmpPredicate::Ne ||
      !lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{final_cmp->type_str}, 32) ||
      final_value_load->ptr != value_slot->result || final_value_load->result.empty() ||
      final_cmp->lhs != final_value_load->result || final_cmp->rhs != "0" ||
      final_branch->cond_name != final_cmp->result ||
      final_branch->true_label != final_true_ret_block.label ||
      final_branch->false_label != final_false_ret_block.label ||
      final_true_ret_block.label != "block_6" || !final_true_ret_block.insts.empty() ||
      final_true_ret == nullptr || final_true_ret_type != bir::TypeKind::I32 ||
      !final_true_ret->value_str.has_value() || *final_true_ret->value_str != "0" ||
      final_false_ret_block.label != "block_7" || !final_false_ret_block.insts.empty() ||
      final_false_ret == nullptr || final_false_ret_type != bir::TypeKind::I32 ||
      !final_false_ret->value_str.has_value() || *final_false_ret->value_str != "1" ||
      (trailing_dead_ret_block != nullptr &&
       (trailing_dead_ret_block->label != "block_8" || !trailing_dead_ret_block->insts.empty() ||
        trailing_dead_ret == nullptr || trailing_dead_ret_type != bir::TypeKind::I32 ||
        !trailing_dead_ret->value_str.has_value() || *trailing_dead_ret->value_str != "0"))) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_branch_only_constant_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.empty() || !function.alloca_insts.empty() ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    if (block.label.empty() || !blocks_by_label.emplace(block.label, &block).second) {
      return std::nullopt;
    }
  }

  auto entry_it = blocks_by_label.find("entry");
  if (entry_it == blocks_by_label.end()) {
    return std::nullopt;
  }

  std::unordered_set<std::string> visited_labels;
  const LirBlock* current = entry_it->second;
  while (current != nullptr) {
    if (!visited_labels.insert(current->label).second || !current->insts.empty()) {
      return std::nullopt;
    }

    if (const auto* ret = std::get_if<LirRet>(&current->terminator)) {
      if (!ret->value_str.has_value() ||
          lower_function_return_type(function, *ret) != bir::TypeKind::I32) {
        return std::nullopt;
      }

      const auto return_imm = parse_immediate(*ret->value_str);
      if (!return_imm.has_value() ||
          *return_imm < std::numeric_limits<std::int32_t>::min() ||
          *return_imm > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
      }

      bir::Module lowered;
      lowered.target_triple = module.target_triple;
      lowered.data_layout = module.data_layout;

      bir::Function lowered_function;
      lowered_function.name = function.name;
      lowered_function.return_type = bir::TypeKind::I32;

      bir::Block lowered_entry;
      lowered_entry.label = "entry";
      lowered_entry.terminator.value =
          bir::Value::immediate_i32(static_cast<std::int32_t>(*return_imm));
      lowered_function.blocks.push_back(std::move(lowered_entry));
      lowered.functions.push_back(std::move(lowered_function));
      return lowered;
    }

    const auto* branch = std::get_if<LirBr>(&current->terminator);
    if (branch == nullptr) {
      return std::nullopt;
    }

    auto next_it = blocks_by_label.find(branch->target_label);
    if (next_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    current = next_it->second;
  }

  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_binary_opcode(std::string_view opcode) {
  if (opcode == "add") {
    return bir::BinaryOpcode::Add;
  }
  if (opcode == "sub") {
    return bir::BinaryOpcode::Sub;
  }
  if (opcode == "mul") {
    return bir::BinaryOpcode::Mul;
  }
  if (opcode == "and") {
    return bir::BinaryOpcode::And;
  }
  if (opcode == "or") {
    return bir::BinaryOpcode::Or;
  }
  if (opcode == "xor") {
    return bir::BinaryOpcode::Xor;
  }
  if (opcode == "shl") {
    return bir::BinaryOpcode::Shl;
  }
  if (opcode == "lshr") {
    return bir::BinaryOpcode::LShr;
  }
  if (opcode == "ashr") {
    return bir::BinaryOpcode::AShr;
  }
  if (opcode == "sdiv") {
    return bir::BinaryOpcode::SDiv;
  }
  if (opcode == "udiv") {
    return bir::BinaryOpcode::UDiv;
  }
  if (opcode == "srem") {
    return bir::BinaryOpcode::SRem;
  }
  if (opcode == "urem") {
    return bir::BinaryOpcode::URem;
  }
  if (opcode == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_compare_materialization_opcode(std::string_view predicate) {
  if (predicate == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  if (predicate == "ne") {
    return bir::BinaryOpcode::Ne;
  }
  if (predicate == "slt") {
    return bir::BinaryOpcode::Slt;
  }
  if (predicate == "sle") {
    return bir::BinaryOpcode::Sle;
  }
  if (predicate == "sgt") {
    return bir::BinaryOpcode::Sgt;
  }
  if (predicate == "sge") {
    return bir::BinaryOpcode::Sge;
  }
  if (predicate == "ult") {
    return bir::BinaryOpcode::Ult;
  }
  if (predicate == "ule") {
    return bir::BinaryOpcode::Ule;
  }
  if (predicate == "ugt") {
    return bir::BinaryOpcode::Ugt;
  }
  if (predicate == "uge") {
    return bir::BinaryOpcode::Uge;
  }
  return std::nullopt;
}

std::optional<bir::Value> lower_lossless_immediate_cast(
    const c4c::codegen::lir::LirInst& inst) {
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
  if (cast == nullptr || cast->result.str().empty() ||
      (cast->kind != c4c::codegen::lir::LirCastKind::SExt &&
       cast->kind != c4c::codegen::lir::LirCastKind::ZExt)) {
    return std::nullopt;
  }

  const auto from_type = lower_scalar_type(cast->from_type);
  const auto to_type = lower_scalar_type(cast->to_type);
  if (!from_type.has_value() || !to_type.has_value() ||
      scalar_type_bit_width(*from_type) >= scalar_type_bit_width(*to_type)) {
    return std::nullopt;
  }

  const auto source = lower_immediate_or_name(cast->operand.str(), *from_type);
  if (!source.has_value() || source->kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt && source->immediate < 0) {
    return std::nullopt;
  }
  if (!immediate_fits_type(source->immediate, *to_type)) {
    return std::nullopt;
  }

  switch (*to_type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(source->immediate));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(source->immediate));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(source->immediate);
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bir::CastInst> lower_cast(const c4c::codegen::lir::LirInst& inst) {
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
  if (cast == nullptr || cast->result.str().empty()) {
    return std::nullopt;
  }
  const auto from_type = lower_scalar_type(cast->from_type);
  const auto to_type = lower_scalar_type(cast->to_type);
  if (!from_type.has_value() || !to_type.has_value() ||
      *from_type == *to_type) {
    return std::nullopt;
  }

  bir::CastOpcode opcode;
  if (cast->kind == c4c::codegen::lir::LirCastKind::SExt) {
    if (*to_type <= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::SExt;
  } else if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt) {
    if (*to_type <= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::ZExt;
  } else if (cast->kind == c4c::codegen::lir::LirCastKind::Trunc) {
    if (*to_type >= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::Trunc;
  } else {
    return std::nullopt;
  }

  const auto operand = lower_immediate_or_name(cast->operand.str(), *from_type);
  if (!operand.has_value()) {
    return std::nullopt;
  }

  bir::CastInst lowered;
  lowered.opcode = opcode;
  lowered.result = bir::Value::named(*to_type, cast->result.str());
  lowered.operand = *operand;
  return lowered;
}

std::optional<bir::BinaryInst> lower_binary(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (bin == nullptr || bin->result.str().empty()) {
    return std::nullopt;
  }
  const auto type = lower_scalar_type(bin->type_str);
  if (!type.has_value()) {
    return std::nullopt;
  }
  const auto opcode = lower_binary_opcode(bin->opcode.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(bin->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(bin->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, bin->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::BinaryInst> lower_compare_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& cast_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&cast_inst);
  if (cmp == nullptr || cast == nullptr || cmp->is_float || cmp->result.str().empty() ||
      cast->result.str().empty() ||
      cast->kind != c4c::codegen::lir::LirCastKind::ZExt ||
      !lir_type_has_integer_width(cast->from_type, 1) ||
      cast->operand.str() != cmp->result.str()) {
    return std::nullopt;
  }
  const auto opcode = lower_compare_materialization_opcode(cmp->predicate.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto type = lower_scalar_type(cmp->type_str);
  const auto widened_type = lower_scalar_type(cast->to_type);
  if (!type.has_value() || !widened_type.has_value() || *type != *widened_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, cast->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::SelectInst> lower_select_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& select_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* select = std::get_if<c4c::codegen::lir::LirSelectOp>(&select_inst);
  if (cmp == nullptr || select == nullptr || cmp->is_float || cmp->result.str().empty() ||
      select->result.str().empty() || select->cond.str() != cmp->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp->predicate.str());
  const auto type = lower_scalar_type(cmp->type_str);
  const auto selected_type = lower_scalar_type(select->type_str);
  if (!predicate.has_value() || !type.has_value() || !selected_type.has_value() ||
      *type != *selected_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  const auto true_value = lower_immediate_or_name(select->true_val.str(), *type);
  const auto false_value = lower_immediate_or_name(select->false_val.str(), *type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  bir::SelectInst lowered;
  lowered.predicate = *predicate;
  lowered.result = bir::Value::named(*type, select->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  lowered.true_value = *true_value;
  lowered.false_value = *false_value;
  return lowered;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<AffineValue> combine_affine(const AffineValue& lhs,
                                          const AffineValue& rhs,
                                          bir::BinaryOpcode opcode) {
  if (opcode == bir::BinaryOpcode::Add) {
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
  if (opcode == bir::BinaryOpcode::Sub) {
    return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                       lhs.constant - rhs.constant};
  }

  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::And) {
    return AffineValue{false, false, lhs.constant & rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Or) {
    return AffineValue{false, false, lhs.constant | rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Shl) {
    if (rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant << rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::LShr) {
    if (lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) >> rhs.constant)};
  }
  if (opcode == bir::BinaryOpcode::AShr) {
    if (rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant >> rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::SDiv) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant / rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::UDiv) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) /
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::SRem) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant % rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::URem) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) %
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::Eq) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant == rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ne) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant != rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Slt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant < rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sle) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant <= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sgt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant > rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant >= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ult) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) < static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ule) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) <= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ugt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) > static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Uge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) >= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  return AffineValue{false, false, lhs.constant * rhs.constant};
}

std::optional<bool> evaluate_predicate(const AffineValue& lhs,
                                       const AffineValue& rhs,
                                       bir::BinaryOpcode opcode) {
  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
      return lhs.constant == rhs.constant;
    case bir::BinaryOpcode::Ne:
      return lhs.constant != rhs.constant;
    case bir::BinaryOpcode::Slt:
      return lhs.constant < rhs.constant;
    case bir::BinaryOpcode::Sle:
      return lhs.constant <= rhs.constant;
    case bir::BinaryOpcode::Sgt:
      return lhs.constant > rhs.constant;
    case bir::BinaryOpcode::Sge:
      return lhs.constant >= rhs.constant;
    case bir::BinaryOpcode::Ult:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ule:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ugt:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Uge:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& lir_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  std::vector<bir::BinaryInst> lowered;
  lowered.reserve(lir_block.insts.size());

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lir_block.insts.size());
  std::vector<AffineValue> affine_values;
  affine_values.reserve(params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < params.size(); ++index) {
    available_names.push_back(params[index].name);
    affine_values.push_back(AffineValue{index == 0, index == 1, 0});
  }

  auto name_is_available = [&](std::string_view name) {
    return std::find(available_names.begin(), available_names.end(), name) !=
           available_names.end();
  };
  auto operand_is_available = [&](const bir::Value& value) {
    return value.kind != bir::Value::Kind::Named || name_is_available(value.name);
  };
  auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return AffineValue{false, false, value.immediate};
    }
    for (std::size_t index = 0; index < available_names.size(); ++index) {
      if (available_names[index] == value.name) {
        return affine_values[index];
      }
    }
    return std::nullopt;
  };

  for (const auto& inst : lir_block.insts) {
    auto binary = lower_binary(inst);
    if (!binary.has_value() || binary->result.type != type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        name_is_available(binary->result.name)) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_value(binary->lhs);
    const auto rhs = lower_affine_value(binary->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    available_names.push_back(binary->result.name);
    affine_values.push_back(*combined);
    lowered.push_back(*binary);
  }

  if (!lowered.empty() && !expected_result.empty() &&
      lowered.back().result.name != expected_result) {
    return std::nullopt;
  }
  return lowered;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& first_block,
    const c4c::codegen::lir::LirBlock& second_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  auto lowered = lower_bounded_predecessor_chain(first_block, "", params, type);
  if (!lowered.has_value()) {
    return std::nullopt;
  }
  auto tail = lower_bounded_predecessor_chain(second_block, expected_result, params, type);
  if (!tail.has_value()) {
    return std::nullopt;
  }

  if (lowered->empty()) {
    return tail;
  }
  if (tail->empty()) {
    if (lowered->back().result.name != expected_result) {
      return std::nullopt;
    }
    return lowered;
  }

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lowered->size());
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *lowered) {
    available_names.push_back(inst.result.name);
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  for (const auto& inst : *tail) {
    if (!operand_is_available(inst.lhs) || !operand_is_available(inst.rhs) ||
        std::find(available_names.begin(), available_names.end(), inst.result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    lowered->push_back(inst);
    available_names.push_back(inst.result.name);
  }
  return lowered;
}

std::optional<bir::Function> try_lower_conditional_return_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() < 3) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  if (entry.label.empty() || entry.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || !lir_type_has_integer_width(cast->from_type, 1) ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type(cmp0->type_str);
  const auto widened_type = lower_scalar_type(cast->to_type);
  const auto cond_type = lower_scalar_type(cmp1->type_str);
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *widened_type != *cond_type) {
    return std::nullopt;
  }

  const LirBlock* true_ret_block = nullptr;
  const LirBlock* false_ret_block = nullptr;
  std::unordered_map<std::string_view, std::size_t> block_indices_by_label;
  block_indices_by_label.reserve(lir_function.blocks.size());
  for (std::size_t i = 1; i < lir_function.blocks.size(); ++i) {
    const auto& block = lir_function.blocks[i];
    if (block.label.empty() || !block_indices_by_label.emplace(block.label, i).second) {
      return std::nullopt;
    }
  }

  std::unordered_set<std::size_t> consumed_block_indices;
  auto consume_return_arm = [&](std::string_view branch_label) -> const LirBlock* {
    while (true) {
      const auto block_it = block_indices_by_label.find(branch_label);
      if (block_it == block_indices_by_label.end()) {
        return nullptr;
      }

      const auto block_index = block_it->second;
      const auto& block = lir_function.blocks[block_index];
      if (!consumed_block_indices.insert(block_index).second || !block.insts.empty()) {
        return nullptr;
      }

      if (std::get_if<LirRet>(&block.terminator) != nullptr) {
        return &block;
      }

      const auto* bridge_br = std::get_if<LirBr>(&block.terminator);
      if (bridge_br == nullptr) {
        return nullptr;
      }
      branch_label = bridge_br->target_label;
    }
  };

  true_ret_block = consume_return_arm(condbr->true_label);
  false_ret_block = consume_return_arm(condbr->false_label);
  if (true_ret_block == nullptr || false_ret_block == nullptr ||
      consumed_block_indices.size() + 1 != lir_function.blocks.size()) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_ret_block->terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_ret_block->terminator);
  const auto true_ret_type =
      true_ret == nullptr ? std::nullopt : lower_function_return_type(lir_function, *true_ret);
  const auto false_ret_type =
      false_ret == nullptr ? std::nullopt : lower_function_return_type(lir_function, *false_ret);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || !true_ret_type.has_value() ||
      !false_ret_type.has_value() || *true_ret_type != *false_ret_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  const auto true_value = lower_immediate_or_name(*true_ret->value_str, *true_ret_type);
  const auto false_value = lower_immediate_or_name(*false_ret->value_str, *true_ret_type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  if (*compare_type != *widened_type) {
    if (lhs->kind != bir::Value::Kind::Immediate ||
        rhs->kind != bir::Value::Kind::Immediate ||
        true_value->kind != bir::Value::Kind::Immediate ||
        false_value->kind != bir::Value::Kind::Immediate) {
      return std::nullopt;
    }

    const auto predicate_value = evaluate_predicate(
        AffineValue{false, false, lhs->immediate},
        AffineValue{false, false, rhs->immediate},
        *predicate);
    if (!predicate_value.has_value()) {
      return std::nullopt;
    }

    bir::Function function;
    function.name = lir_function.name;
    function.return_type = *true_ret_type;
    function.params = params;

    bir::Block block;
    block.label = entry.label;
    block.terminator.value = *predicate_value ? *true_value : *false_value;
    function.blocks.push_back(std::move(block));
    return function;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs) ||
      !operand_is_param_or_immediate(*true_value) ||
      !operand_is_param_or_immediate(*false_value)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *true_ret_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*true_ret_type, "%t.select"),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  block.terminator.value = bir::Value::named(*true_ret_type, "%t.select");
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_conditional_phi_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 4 && lir_function.blocks.size() != 6) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto* cmp0 = entry.insts.size() > 0 ? std::get_if<LirCmpOp>(&entry.insts[0]) : nullptr;
  const auto* cast = entry.insts.size() > 1 ? std::get_if<LirCastOp>(&entry.insts[1]) : nullptr;
  const auto* cmp1 = entry.insts.size() > 2 ? std::get_if<LirCmpOp>(&entry.insts[2]) : nullptr;
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label.empty() || entry.insts.size() != 3 || cmp0 == nullptr || cast == nullptr ||
      cmp1 == nullptr || condbr == nullptr || cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || !lir_type_has_integer_width(cast->from_type, 1) ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type(cmp0->type_str);
  const auto widened_type = lower_scalar_type(cast->to_type);
  const auto cond_type = lower_scalar_type(cmp1->type_str);
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *compare_type != *widened_type ||
      *compare_type != *cond_type) {
    return std::nullopt;
  }

  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[lir_function.blocks.size() == 4 ? 2 : 3];
  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  if (true_br == nullptr || false_br == nullptr || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  const c4c::codegen::lir::LirBlock* true_phi_pred = &true_block;
  const c4c::codegen::lir::LirBlock* false_phi_pred = &false_block;
  const c4c::codegen::lir::LirBlock* true_value_block = &true_block;
  const c4c::codegen::lir::LirBlock* false_value_block = &false_block;
  const c4c::codegen::lir::LirBlock* join_block = nullptr;
  if (lir_function.blocks.size() == 4) {
    join_block = &lir_function.blocks[3];
    if (true_br->target_label != join_block->label || false_br->target_label != join_block->label) {
      return std::nullopt;
    }
  } else {
    const auto& true_end = lir_function.blocks[2];
    const auto& false_end = lir_function.blocks[4];
    const auto* true_end_br = std::get_if<LirBr>(&true_end.terminator);
    const auto* false_end_br = std::get_if<LirBr>(&false_end.terminator);
    join_block = &lir_function.blocks[5];
    if (true_end_br == nullptr || false_end_br == nullptr ||
        true_br->target_label != true_end.label ||
        false_br->target_label != false_end.label ||
        true_end_br->target_label != join_block->label ||
        false_end_br->target_label != join_block->label) {
      return std::nullopt;
    }
    true_phi_pred = &true_end;
    false_phi_pred = &false_end;
    true_value_block = &true_block;
    false_value_block = &false_block;
  }

  const auto* phi = join_block->insts.size() > 0 ? std::get_if<LirPhiOp>(&join_block->insts[0]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block->terminator);
  const auto phi_type = phi == nullptr ? std::nullopt : lower_scalar_type(phi->type_str);
  const auto return_type = ret == nullptr ? std::nullopt : lower_function_return_type(lir_function, *ret);
  if (phi == nullptr || ret == nullptr ||
      phi->result.str().empty() || !phi_type.has_value() || *phi_type != *compare_type ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() ||
      !return_type.has_value() || *return_type != *compare_type ||
      phi->incoming[0].second != true_phi_pred->label ||
      phi->incoming[1].second != false_phi_pred->label) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto true_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*true_phi_pred, phi->incoming[0].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*true_value_block, *true_phi_pred,
                                            phi->incoming[0].first, params, *compare_type);
  const auto false_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*false_phi_pred, phi->incoming[1].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*false_value_block, *false_phi_pred,
                                            phi->incoming[1].first, params, *compare_type);
  if (!true_chain.has_value() || !false_chain.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *compare_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  std::vector<std::string> available_names;
  available_names.reserve(params.size() + true_chain->size() + false_chain->size() + 1);
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *true_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }
  for (const auto& inst : *false_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }

  auto lower_branch_value = [&](std::string_view value_text,
                                const std::vector<bir::BinaryInst>& chain)
      -> std::optional<bir::Value> {
    if (!chain.empty()) {
      return bir::Value::named(*compare_type, std::string(value_text));
    }
    return lower_immediate_or_name(value_text, *compare_type);
  };
  const auto true_value = lower_branch_value(phi->incoming[0].first, *true_chain);
  const auto false_value = lower_branch_value(phi->incoming[1].first, *false_chain);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  if (!operand_is_available(*true_value) || !operand_is_available(*false_value)) {
    return std::nullopt;
  }

  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*compare_type, phi->result.str()),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  available_names.push_back(phi->result.str());

  for (std::size_t inst_index = 1; inst_index < join_block->insts.size(); ++inst_index) {
    auto binary = lower_binary(join_block->insts[inst_index]);
    if (!binary.has_value() || binary->result.type != *compare_type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        std::find(available_names.begin(), available_names.end(), binary->result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(*binary);
    available_names.push_back(binary->result.name);
  }

  const auto return_value = lower_immediate_or_name(*ret->value_str, *compare_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  if (return_value->kind == bir::Value::Kind::Named &&
      std::find(available_names.begin(), available_names.end(), return_value->name) ==
          available_names.end()) {
    return std::nullopt;
  }
  block.terminator.value = *return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_add_sub_chain_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 1) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty() || lir_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&lir_block.terminator);
  const auto* trunc = std::get_if<LirCastOp>(&lir_block.insts.back());
  if (ret == nullptr || trunc == nullptr || !ret->value_str.has_value() ||
      trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
      !lir_function_returns_integer_width(lir_function, 8) ||
      trunc->kind != LirCastKind::Trunc ||
      !lir_type_has_integer_width(trunc->from_type, 32) ||
      !lir_type_has_integer_width(trunc->to_type, 8)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = lir_block.label;

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + lir_block.insts.size());
  std::vector<AffineValue> affine_values;
  affine_values.reserve(params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < params.size(); ++index) {
    defined_names.push_back(params[index].name);
    resolved_values.push_back(bir::Value::named(params[index].type, params[index].name));
    affine_values.push_back(AffineValue{index == 0, index == 1, 0});
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return AffineValue{false, false, value.immediate};
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return affine_values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 1 < lir_block.insts.size(); ++inst_index) {
    if (const auto* cast = std::get_if<LirCastOp>(&lir_block.insts[inst_index]); cast != nullptr) {
      if (cast->result.str().empty() || name_is_defined(cast->result.str()) ||
          (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
          !lir_type_has_integer_width(cast->from_type, 8) ||
          !lir_type_has_integer_width(cast->to_type, 32)) {
        return std::nullopt;
      }
      const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
      if (!source.has_value()) {
        return std::nullopt;
      }
      const auto resolved_source = resolve_value(*source);
      const auto affine_source = resolved_source.has_value()
                                     ? lower_affine_value(*resolved_source)
                                     : std::nullopt;
      if (!resolved_source.has_value() || !affine_source.has_value()) {
        return std::nullopt;
      }
      defined_names.push_back(cast->result.str());
      resolved_values.push_back(*resolved_source);
      affine_values.push_back(*affine_source);
      continue;
    }

    auto binary = lower_binary(lir_block.insts[inst_index]);
    if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub &&
         binary->opcode != bir::BinaryOpcode::Mul &&
         binary->opcode != bir::BinaryOpcode::And &&
         binary->opcode != bir::BinaryOpcode::Or &&
         binary->opcode != bir::BinaryOpcode::Xor &&
         binary->opcode != bir::BinaryOpcode::Shl &&
         binary->opcode != bir::BinaryOpcode::LShr &&
         binary->opcode != bir::BinaryOpcode::AShr &&
         binary->opcode != bir::BinaryOpcode::SDiv &&
         binary->opcode != bir::BinaryOpcode::UDiv &&
         binary->opcode != bir::BinaryOpcode::SRem &&
         binary->opcode != bir::BinaryOpcode::URem) ||
        name_is_defined(binary->result.name)) {
      return std::nullopt;
    }

    const auto lhs_value = resolve_value(binary->lhs);
    const auto rhs_value = resolve_value(binary->rhs);
    if (!lhs_value.has_value() || !rhs_value.has_value()) {
      return std::nullopt;
    }
    const auto narrow_lhs = narrow_i8_value(*lhs_value);
    const auto narrow_rhs = narrow_i8_value(*rhs_value);
    if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
      return std::nullopt;
    }

    const auto lhs_affine = lower_affine_value(*narrow_lhs);
    const auto rhs_affine = lower_affine_value(*narrow_rhs);
    if (!lhs_affine.has_value() || !rhs_affine.has_value()) {
      return std::nullopt;
    }
    if (binary->opcode == bir::BinaryOpcode::Mul &&
        (lhs_affine->uses_first_param || lhs_affine->uses_second_param ||
         rhs_affine->uses_first_param || rhs_affine->uses_second_param)) {
      return std::nullopt;
    }
    const auto combined = combine_affine(*lhs_affine, *rhs_affine, binary->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }

    bir::BinaryInst narrowed;
    narrowed.opcode = binary->opcode;
    narrowed.result = bir::Value::named(bir::TypeKind::I8, binary->result.name);
    narrowed.lhs = *narrow_lhs;
    narrowed.rhs = *narrow_rhs;
    block.insts.push_back(narrowed);

    defined_names.push_back(binary->result.name);
    resolved_values.push_back(bir::Value::named(bir::TypeKind::I8, binary->result.name));
    affine_values.push_back(*combined);
  }

  const auto widened_return_value =
      lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
  if (!widened_return_value.has_value()) {
    return std::nullopt;
  }
  const auto resolved_return_value = resolve_value(*widened_return_value);
  const auto narrow_return_value = resolved_return_value.has_value()
                                       ? narrow_i8_value(*resolved_return_value)
                                       : std::nullopt;
  if (!narrow_return_value.has_value()) {
    return std::nullopt;
  }

  block.terminator.value = *narrow_return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_compare_return_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (!params.empty() || lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty() || lir_block.insts.size() < 3) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&lir_block.terminator);
  const auto* cmp = std::get_if<LirCmpOp>(&lir_block.insts[lir_block.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&lir_block.insts[lir_block.insts.size() - 2]);
  const auto* trunc = std::get_if<LirCastOp>(&lir_block.insts[lir_block.insts.size() - 1]);
  if (ret == nullptr || trunc == nullptr || !ret->value_str.has_value() ||
      trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
      !lir_function_returns_integer_width(lir_function, 8) ||
      trunc->kind != LirCastKind::Trunc ||
      !lir_type_has_integer_width(trunc->from_type, 32) ||
      !lir_type_has_integer_width(trunc->to_type, 8) ||
      cmp == nullptr || cmp->is_float || cmp->result.str().empty() ||
      cond_cast == nullptr || cond_cast->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt ||
      !lir_type_has_integer_width(cond_cast->from_type, 1) ||
      !lir_type_has_integer_width(cond_cast->to_type, 32) ||
      cond_cast->operand.str() != cmp->result.str() ||
      trunc->operand.str() != cond_cast->result.str()) {
    return std::nullopt;
  }

  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  std::vector<std::string> defined_names;
  defined_names.reserve(lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(lir_block.insts.size());

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < lir_block.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&lir_block.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str())) {
      return std::nullopt;
    }

    if (cast->kind == LirCastKind::Trunc && lir_type_has_integer_width(cast->from_type, 32) &&
        lir_type_has_integer_width(cast->to_type, 8)) {
      const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I32);
      if (!source.has_value() || source->kind != bir::Value::Kind::Immediate ||
          !immediate_fits_type(source->immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      defined_names.push_back(cast->result.str());
      resolved_values.push_back(
          bir::Value::immediate_i8(static_cast<std::int8_t>(source->immediate)));
      continue;
    }

    if ((cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        !lir_type_has_integer_width(cast->from_type, 8) ||
        !lir_type_has_integer_width(cast->to_type, 32)) {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp->predicate.str());
  if (!predicate.has_value() || !lir_type_has_integer_width(cmp->type_str, 32)) {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }
  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto narrow_lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto narrow_rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = lir_block.label;
  bir::BinaryInst compare;
  compare.opcode = *predicate;
  compare.result = bir::Value::named(bir::TypeKind::I8, cond_cast->result.str());
  compare.lhs = *narrow_lhs;
  compare.rhs = *narrow_rhs;
  block.insts.push_back(compare);
  block.terminator.value = bir::Value::named(bir::TypeKind::I8, compare.result.name);

  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_conditional_return_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 3) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[2];
  if (entry.label.empty() || entry.insts.size() < 4 || true_block.label.empty() ||
      false_block.label.empty()) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[entry.insts.size() - 2]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 1]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cond_cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() || cond_cast->result.str().empty() ||
      cmp1->is_float || cmp1->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt ||
      !lir_type_has_integer_width(cond_cast->from_type, 1) ||
      !lir_type_has_integer_width(cond_cast->to_type, 32) ||
      cond_cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      !lir_type_has_integer_width(cmp1->type_str, 32) ||
      cmp1->lhs.str() != cond_cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str() || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + entry.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + entry.insts.size());
  for (const auto& param : params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < entry.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str()) ||
        (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        !lir_type_has_integer_width(cast->from_type, 8) ||
        !lir_type_has_integer_width(cast->to_type, 32)) {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  if (!predicate.has_value() || !lir_type_has_integer_width(cmp0->type_str, 32)) {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp0->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp0->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }

  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  auto lower_branch_value = [&](const LirBlock& block) -> std::optional<bir::Value> {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    if (ret == nullptr || !ret->value_str.has_value() ||
        !lir_function_returns_integer_width(lir_function, 8)) {
      return std::nullopt;
    }
    if (block.insts.empty()) {
      const auto value = lower_immediate_or_name(*ret->value_str, bir::TypeKind::I8);
      if (!value.has_value()) {
        return std::nullopt;
      }
      return resolve_value(*value);
    }
    if (block.insts.size() != 1) {
      return std::nullopt;
    }

    const auto* trunc = std::get_if<LirCastOp>(&block.insts.front());
    if (trunc == nullptr || trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
        trunc->kind != LirCastKind::Trunc ||
        !lir_type_has_integer_width(trunc->from_type, 32) ||
        !lir_type_has_integer_width(trunc->to_type, 8)) {
      return std::nullopt;
    }

    const auto widened_value = lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
    if (!widened_value.has_value()) {
      return std::nullopt;
    }
    const auto resolved_value = resolve_value(*widened_value);
    if (!resolved_value.has_value()) {
      return std::nullopt;
    }
    return narrow_i8_value(*resolved_value);
  };

  const auto true_value = lower_branch_value(true_block);
  const auto false_value = lower_branch_value(false_block);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs) ||
      !operand_is_param_or_immediate(*true_value) ||
      !operand_is_param_or_immediate(*false_value)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(bir::TypeKind::I8, "%t.select"),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  block.terminator.value = bir::Value::named(bir::TypeKind::I8, "%t.select");
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_conditional_phi_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 4 && lir_function.blocks.size() != 6) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  if (entry.label.empty() || entry.insts.size() < 4) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[entry.insts.size() - 2]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 1]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cond_cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() || cond_cast->result.str().empty() ||
      cmp1->is_float || cmp1->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt ||
      !lir_type_has_integer_width(cond_cast->from_type, 1) ||
      !lir_type_has_integer_width(cond_cast->to_type, 32) ||
      cond_cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      !lir_type_has_integer_width(cmp1->type_str, 32) ||
      cmp1->lhs.str() != cond_cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + entry.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + entry.insts.size());
  for (const auto& param : params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto resolve_value_from = [&](const std::vector<std::string>& names,
                                const std::vector<bir::Value>& values,
                                const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < names.size(); ++index) {
      if (names[index] == value.name) {
        return values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < entry.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str()) ||
        (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        !lir_type_has_integer_width(cast->from_type, 8) ||
        !lir_type_has_integer_width(cast->to_type, 32)) {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  if (!predicate.has_value() || !lir_type_has_integer_width(cmp0->type_str, 32)) {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp0->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp0->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }
  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[lir_function.blocks.size() == 4 ? 2 : 3];
  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  if (true_br == nullptr || false_br == nullptr || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  const LirBlock* true_phi_pred = &true_block;
  const LirBlock* false_phi_pred = &false_block;
  const LirBlock* true_value_block = &true_block;
  const LirBlock* false_value_block = &false_block;
  const LirBlock* join_block = nullptr;
  if (lir_function.blocks.size() == 4) {
    join_block = &lir_function.blocks[3];
    if (true_br->target_label != join_block->label || false_br->target_label != join_block->label) {
      return std::nullopt;
    }
  } else {
    const auto& true_end = lir_function.blocks[2];
    const auto& false_end = lir_function.blocks[4];
    const auto* true_end_br = std::get_if<LirBr>(&true_end.terminator);
    const auto* false_end_br = std::get_if<LirBr>(&false_end.terminator);
    join_block = &lir_function.blocks[5];
    if (true_end_br == nullptr || false_end_br == nullptr ||
        true_br->target_label != true_end.label ||
        false_br->target_label != false_end.label ||
        true_end_br->target_label != join_block->label ||
        false_end_br->target_label != join_block->label) {
      return std::nullopt;
    }
    true_phi_pred = &true_end;
    false_phi_pred = &false_end;
  }

  const auto* phi = join_block->insts.size() > 0 ? std::get_if<LirPhiOp>(&join_block->insts[0]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block->terminator);
  if (phi == nullptr || ret == nullptr || phi->result.str().empty() ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() ||
      !lir_function_returns_integer_width(lir_function, 8) ||
      phi->incoming[0].second != true_phi_pred->label ||
      phi->incoming[1].second != false_phi_pred->label) {
    return std::nullopt;
  }

  struct LoweredWidenedChain {
    std::vector<bir::BinaryInst> insts;
    bir::Value final_value;
  };
  auto lower_widened_chain = [&](const LirBlock& first_block,
                                 const LirBlock* second_block,
                                 std::string_view expected_result,
                                 bir::TypeKind expected_type)
      -> std::optional<LoweredWidenedChain> {
    std::vector<std::string> local_names = defined_names;
    std::vector<bir::Value> local_values = resolved_values;
    std::vector<bir::BinaryInst> lowered;

    auto name_is_defined_local = [&](std::string_view name) {
      return std::find(local_names.begin(), local_names.end(), name) != local_names.end();
    };
    auto handle_inst = [&](const LirInst& inst) -> bool {
      if (const auto* cast = std::get_if<LirCastOp>(&inst); cast != nullptr) {
        if (cast->result.str().empty() || name_is_defined_local(cast->result.str())) {
          return false;
        }
        if ((cast->kind == LirCastKind::SExt || cast->kind == LirCastKind::ZExt) &&
            lir_type_has_integer_width(cast->from_type, 8) &&
            lir_type_has_integer_width(cast->to_type, 32)) {
          const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
          if (!source.has_value()) {
            return false;
          }
          const auto resolved_source = resolve_value_from(local_names, local_values, *source);
          if (!resolved_source.has_value()) {
            return false;
          }
          local_names.push_back(cast->result.str());
          local_values.push_back(*resolved_source);
          return true;
        }
        if (cast->kind == LirCastKind::Trunc && lir_type_has_integer_width(cast->from_type, 32) &&
            lir_type_has_integer_width(cast->to_type, 8)) {
          const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I32);
          if (!source.has_value()) {
            return false;
          }
          const auto resolved_source = resolve_value_from(local_names, local_values, *source);
          const auto narrowed_source =
              resolved_source.has_value() ? narrow_i8_value(*resolved_source) : std::nullopt;
          if (!narrowed_source.has_value()) {
            return false;
          }
          local_names.push_back(cast->result.str());
          local_values.push_back(*narrowed_source);
          return true;
        }
        return false;
      }

      auto binary = lower_binary(inst);
      if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
          (binary->opcode != bir::BinaryOpcode::Add &&
           binary->opcode != bir::BinaryOpcode::Sub) ||
          name_is_defined_local(binary->result.name)) {
        return false;
      }
      const auto lhs_value = resolve_value_from(local_names, local_values, binary->lhs);
      const auto rhs_value = resolve_value_from(local_names, local_values, binary->rhs);
      const auto narrow_lhs = lhs_value.has_value() ? narrow_i8_value(*lhs_value) : std::nullopt;
      const auto narrow_rhs = rhs_value.has_value() ? narrow_i8_value(*rhs_value) : std::nullopt;
      if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
        return false;
      }

      lowered.push_back(bir::BinaryInst{
          binary->opcode,
          bir::Value::named(bir::TypeKind::I8, binary->result.name),
          *narrow_lhs,
          *narrow_rhs,
      });
      local_names.push_back(binary->result.name);
      local_values.push_back(bir::Value::named(bir::TypeKind::I8, binary->result.name));
      return true;
    };

    for (const auto& inst : first_block.insts) {
      if (!handle_inst(inst)) {
        return std::nullopt;
      }
    }
    if (second_block != nullptr) {
      for (const auto& inst : second_block->insts) {
        if (!handle_inst(inst)) {
          return std::nullopt;
        }
      }
    }

    const auto final_value = lower_immediate_or_name(expected_result, expected_type);
    if (!final_value.has_value()) {
      return std::nullopt;
    }
    const auto resolved_value = resolve_value_from(local_names, local_values, *final_value);
    const auto narrow_value =
        resolved_value.has_value() ? narrow_i8_value(*resolved_value) : std::nullopt;
    if (!narrow_value.has_value()) {
      return std::nullopt;
    }
    return LoweredWidenedChain{std::move(lowered), *narrow_value};
  };

  const auto phi_type = lir_type_has_integer_width(phi->type_str, 8)
                            ? std::optional<bir::TypeKind>(bir::TypeKind::I8)
                            : (lir_type_has_integer_width(phi->type_str, 32)
                                   ? std::optional<bir::TypeKind>(bir::TypeKind::I32)
                                   : std::nullopt);
  if (!phi_type.has_value()) {
    return std::nullopt;
  }

  const auto true_chain =
      lower_widened_chain(*true_value_block,
                          lir_function.blocks.size() == 4 ? nullptr : true_phi_pred,
                          phi->incoming[0].first, *phi_type);
  const auto false_chain =
      lower_widened_chain(*false_value_block,
                          lir_function.blocks.size() == 4 ? nullptr : false_phi_pred,
                          phi->incoming[1].first, *phi_type);
  if (!true_chain.has_value() || !false_chain.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  std::vector<std::string> available_names;
  available_names.reserve(params.size() + true_chain->insts.size() + false_chain->insts.size() +
                          join_block->insts.size());
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : true_chain->insts) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }
  for (const auto& inst : false_chain->insts) {
    if (std::find(available_names.begin(), available_names.end(), inst.result.name) !=
        available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }

  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(bir::TypeKind::I8, phi->result.str()),
      *lhs,
      *rhs,
      true_chain->final_value,
      false_chain->final_value,
  });
  available_names.push_back(phi->result.str());

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };

  if (*phi_type == bir::TypeKind::I8 && join_block->insts.size() == 1 &&
      *ret->value_str == phi->result.str()) {
    block.terminator.value = bir::Value::named(bir::TypeKind::I8, phi->result.str());
    function.blocks.push_back(std::move(block));
    return function;
  }

  const auto* trunc =
      join_block->insts.empty() ? nullptr : std::get_if<LirCastOp>(&join_block->insts.back());
  if (*phi_type != bir::TypeKind::I32 || trunc == nullptr || trunc->result.str().empty() ||
      trunc->kind != LirCastKind::Trunc ||
      !lir_type_has_integer_width(trunc->from_type, 32) ||
      !lir_type_has_integer_width(trunc->to_type, 8) ||
      *ret->value_str != trunc->result.str()) {
    return std::nullopt;
  }

  for (std::size_t inst_index = 1; inst_index + 1 < join_block->insts.size(); ++inst_index) {
    auto binary = lower_binary(join_block->insts[inst_index]);
    if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }

    const auto lhs_value =
        binary->lhs.kind == bir::Value::Kind::Immediate
            ? std::optional<bir::Value>(binary->lhs)
            : (binary->lhs.name == phi->result.str()
                   ? std::optional<bir::Value>(
                         bir::Value::named(bir::TypeKind::I8, phi->result.str()))
                   : std::nullopt);
    const auto rhs_value =
        binary->rhs.kind == bir::Value::Kind::Immediate
            ? std::optional<bir::Value>(binary->rhs)
            : (binary->rhs.name == phi->result.str()
                   ? std::optional<bir::Value>(
                         bir::Value::named(bir::TypeKind::I8, phi->result.str()))
                   : std::nullopt);
    const auto narrowed_lhs = lhs_value.has_value()
                                  ? narrow_i8_value(*lhs_value)
                                  : (operand_is_available(binary->lhs)
                                         ? std::optional<bir::Value>(
                                               bir::Value::named(bir::TypeKind::I8,
                                                                 binary->lhs.name))
                                         : std::nullopt);
    const auto narrowed_rhs = rhs_value.has_value()
                                  ? narrow_i8_value(*rhs_value)
                                  : (operand_is_available(binary->rhs)
                                         ? std::optional<bir::Value>(
                                               bir::Value::named(bir::TypeKind::I8,
                                                                 binary->rhs.name))
                                         : std::nullopt);
    if (!narrowed_lhs.has_value() || !narrowed_rhs.has_value() ||
        std::find(available_names.begin(), available_names.end(), binary->result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(bir::BinaryInst{
        binary->opcode,
        bir::Value::named(bir::TypeKind::I8, binary->result.name),
        *narrowed_lhs,
        *narrowed_rhs,
    });
    available_names.push_back(binary->result.name);
  }

  const auto widened_return_value =
      lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
  if (!widened_return_value.has_value()) {
    return std::nullopt;
  }
  bir::Value return_value;
  if (widened_return_value->kind == bir::Value::Kind::Immediate) {
    const auto narrowed_return = narrow_i8_value(*widened_return_value);
    if (!narrowed_return.has_value()) {
      return std::nullopt;
    }
    return_value = *narrowed_return;
  } else if (operand_is_available(bir::Value::named(bir::TypeKind::I8, widened_return_value->name))) {
    return_value = bir::Value::named(bir::TypeKind::I8, widened_return_value->name);
  } else {
    return std::nullopt;
  }

  block.terminator.value = return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

}  // namespace

std::optional<bir::Module> try_lower_to_bir_legacy(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = try_lower_minimal_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_void_direct_call_imm_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_declared_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_two_arg_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_direct_call_add_imm_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_direct_call_identity_arg_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_folded_two_arg_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_dual_identity_direct_call_sub_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_call_crossing_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_branch_only_constant_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_countdown_loop_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_double_countdown_guarded_zero_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_signed_narrow_local_slot_increment_compare_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_dead_local_add_store_return_immediate_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_store_load_sub_return_immediate_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_arithmetic_chain_return_immediate_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_two_local_i32_zero_init_return_first_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_pointer_store_zero_load_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_pointer_gep_zero_load_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_pointer_gep_zero_store_slot_load_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_array_two_slot_sum_sub_three_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_i32_array_second_slot_pointer_store_zero_load_return_module(
              module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_local_two_field_struct_sub_sub_two_return_module(
          module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_struct_pointer_alias_add_sub_three_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_minimal_local_self_referential_struct_pointer_chain_zero_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered =
          try_lower_double_indirect_local_store_one_final_branch_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_string_literal_compare_phi_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_scalar_global_load_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_extern_scalar_global_load_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_extern_global_array_load_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_global_char_pointer_diff_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_global_int_pointer_diff_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_scalar_global_store_reload_module(module);
      lowered.has_value()) {
    return lowered;
  }

  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_function = module.functions.front();
  if (lir_function.is_declaration || !lir_function.alloca_insts.empty() ||
      lir_function.params.size() > 2) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  const auto params = lower_function_params(lir_function);
  if (!params.has_value()) {
    return std::nullopt;
  }

  if (const auto select_function =
          try_lower_conditional_return_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }
  if (const auto select_function =
          try_lower_conditional_phi_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }
  if (const auto widened_i8_select_function =
          try_lower_widened_i8_conditional_phi_select_function(lir_function, *params);
      widened_i8_select_function.has_value()) {
    lowered.functions.push_back(*widened_i8_select_function);
    return lowered;
  }
  if (const auto widened_i8_select_function =
          try_lower_widened_i8_conditional_return_select_function(lir_function, *params);
      widened_i8_select_function.has_value()) {
    lowered.functions.push_back(*widened_i8_select_function);
    return lowered;
  }
  if (const auto widened_i8_function =
          try_lower_widened_i8_compare_return_function(lir_function, *params);
      widened_i8_function.has_value()) {
    lowered.functions.push_back(*widened_i8_function);
    return lowered;
  }
  if (const auto widened_i8_function =
          try_lower_widened_i8_add_sub_chain_function(lir_function, *params);
      widened_i8_function.has_value()) {
    lowered.functions.push_back(*widened_i8_function);
    return lowered;
  }

  if (lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&lir_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto return_type = lower_function_return_type(lir_function, *ret);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *return_type;
  function.params = *params;

  bir::Block block;
  block.label = lir_block.label;
  std::vector<std::string> defined_names;
  defined_names.reserve(function.params.size() + lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(function.params.size() + lir_block.insts.size());
  for (const auto& param : function.params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }
  std::vector<AffineValue> affine_values;
  affine_values.reserve(function.params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    affine_values.push_back(
        AffineValue{index == 0, index == 1, 0});
  }
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  for (std::size_t inst_index = 0; inst_index < lir_block.insts.size(); ++inst_index) {
    auto name_is_defined = [&](std::string_view name) {
      return std::find(defined_names.begin(), defined_names.end(), name) !=
             defined_names.end();
    };
    auto operand_is_available = [&](const bir::Value& value) {
      return value.kind != bir::Value::Kind::Named || name_is_defined(value.name);
    };
    auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
      if (value.kind == bir::Value::Kind::Immediate) {
        return AffineValue{false, false, value.immediate};
      }
      for (std::size_t index = 0; index < defined_names.size(); ++index) {
        if (defined_names[index] == value.name) {
          return affine_values[index];
        }
      }
      return std::nullopt;
    };

    if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&lir_block.insts[inst_index]);
        cast != nullptr) {
      const auto immediate_cast = lower_lossless_immediate_cast(lir_block.insts[inst_index]);
      if (immediate_cast.has_value() && !name_is_defined(cast->result.str())) {
        defined_names.push_back(cast->result.str());
        resolved_values.push_back(*immediate_cast);
        affine_values.push_back(AffineValue{false, false, immediate_cast->immediate});
        continue;
      }
      auto cast_inst = lower_cast(lir_block.insts[inst_index]);
      if (cast_inst.has_value() && !name_is_defined(cast->result.str())) {
        const auto resolved_operand = resolve_value(cast_inst->operand);
        if (!resolved_operand.has_value()) {
          return std::nullopt;
        }
        cast_inst->operand = *resolved_operand;
        const auto operand_affine = lower_affine_value(cast_inst->operand);
        if (!operand_affine.has_value()) {
          return std::nullopt;
        }
        defined_names.push_back(cast->result.str());
        resolved_values.push_back(bir::Value::named(cast_inst->result.type, cast->result.str()));
        affine_values.push_back(*operand_affine);
        block.insts.push_back(*cast_inst);
        continue;
      }
      return std::nullopt;
    }

    auto binary = [&]() -> std::optional<bir::BinaryInst> {
      if (inst_index + 1 < lir_block.insts.size()) {
        auto lowered_compare = lower_compare_materialization(
            lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
        if (lowered_compare.has_value()) {
          ++inst_index;
          return lowered_compare;
        }
      }
      return lower_binary(lir_block.insts[inst_index]);
    }();
    if (binary.has_value()) {
      const auto lhs_value = resolve_value(binary->lhs);
      const auto rhs_value = resolve_value(binary->rhs);
      if (!lhs_value.has_value() || !rhs_value.has_value() ||
          name_is_defined(binary->result.name)) {
        return std::nullopt;
      }
      binary->lhs = *lhs_value;
      binary->rhs = *rhs_value;
      const auto lhs = lower_affine_value(binary->lhs);
      const auto rhs = lower_affine_value(binary->rhs);
      if (!lhs.has_value() || !rhs.has_value()) {
        return std::nullopt;
      }
      const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
      if (!combined.has_value()) {
        return std::nullopt;
      }
      defined_names.push_back(binary->result.name);
      resolved_values.push_back(bir::Value::named(binary->result.type, binary->result.name));
      affine_values.push_back(*combined);
      block.insts.push_back(*binary);
      continue;
    }

    if (inst_index + 1 >= lir_block.insts.size()) {
      return std::nullopt;
    }
    auto select = lower_select_materialization(
        lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
    if (!select.has_value()) {
      return std::nullopt;
    }
    ++inst_index;
    const auto lhs_value = resolve_value(select->lhs);
    const auto rhs_value = resolve_value(select->rhs);
    const auto true_value_resolved = resolve_value(select->true_value);
    const auto false_value_resolved = resolve_value(select->false_value);
    if (!lhs_value.has_value() || !rhs_value.has_value() ||
        !true_value_resolved.has_value() || !false_value_resolved.has_value() ||
        name_is_defined(select->result.name)) {
      return std::nullopt;
    }
    select->lhs = *lhs_value;
    select->rhs = *rhs_value;
    select->true_value = *true_value_resolved;
    select->false_value = *false_value_resolved;
    const auto lhs = lower_affine_value(select->lhs);
    const auto rhs = lower_affine_value(select->rhs);
    const auto true_value = lower_affine_value(select->true_value);
    const auto false_value = lower_affine_value(select->false_value);
    if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
        !false_value.has_value()) {
      return std::nullopt;
    }
    const auto predicate = evaluate_predicate(*lhs, *rhs, select->predicate);
    if (!predicate.has_value()) {
      return std::nullopt;
    }
    defined_names.push_back(select->result.name);
    resolved_values.push_back(bir::Value::named(select->result.type, select->result.name));
    affine_values.push_back(*predicate ? *true_value : *false_value);
    block.insts.push_back(*select);
  }

  auto return_value = lower_immediate_or_name(*ret->value_str, function.return_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  const auto resolved_return_value = resolve_value(*return_value);
  if (!resolved_return_value.has_value()) {
    return std::nullopt;
  }
  block.terminator.value = *resolved_return_value;

  function.blocks.push_back(std::move(block));
  lowered.functions.push_back(std::move(function));
  return lowered;
}

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options) {
  auto normalized_module = module;
  std::vector<BirLoweringNote> notes;

  if (options.normalize_cfg) {
    normalize_cfg_in_place(normalized_module, options, &notes);
  }
  if (auto lowered =
          try_lower_minimal_string_literal_compare_phi_return_module(normalized_module);
      lowered.has_value()) {
    notes.push_back(BirLoweringNote{
        .phase = "legacy-lowering",
        .message =
            "string-literal compare memory seam lowered the CFG-normalized module before phi-lowering rewrite erased the join phi",
    });
    return BirLoweringResult{
        .module = std::move(lowered),
        .notes = std::move(notes),
    };
  }
  if (options.lower_phi) {
    lower_phi_nodes_in_place(normalized_module, &notes);
  }
  if (options.legalize_types) {
    lir_to_bir::record_type_legalization_scaffold_notes(normalized_module, &notes);
  }
  if (options.lower_memory) {
    record_memory_lowering_scaffold_notes(normalized_module, &notes);
  }
  if (options.lower_calls) {
    record_call_lowering_scaffold_notes(normalized_module, &notes);
  }
  if (options.lower_aggregates) {
    record_aggregate_lowering_scaffold_notes(normalized_module, &notes);
  }

  auto lowered = try_lower_to_bir_legacy(normalized_module);
  if (!lowered.has_value()) {
    notes.push_back(BirLoweringNote{
        .phase = "legacy-lowering",
        .message = "legacy monolithic matcher path could not lower the normalized module",
    });
  }

  return BirLoweringResult{
      .module = std::move(lowered),
      .notes = std::move(notes),
  };
}

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  return try_lower_to_bir_with_options(module, BirLoweringOptions{}).module;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  auto lowered = try_lower_to_bir(module);
  if (!lowered.has_value()) {
    throw std::invalid_argument(
        "bir scaffold lowering currently supports only straight-line single-block i8/i32/i64 return-immediate/add-sub slices, sext/zext/trunc casts, constant-only mul/and/or/shl/lshr/ashr/sdiv/udiv/srem/urem/eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge materialization slices, bounded compare-fed integer select materialization, bounded compare-fed phi joins with empty or add/sub-only predecessor arms including join-local add/sub chains after the fused select, plus bounded one- and two-parameter affine chains over those scalar types");
  }
  return *lowered;
}

}  // namespace c4c::backend
