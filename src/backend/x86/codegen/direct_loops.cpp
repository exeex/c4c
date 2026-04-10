#include "x86_codegen.hpp"

#include "../../bir.hpp"

#include <optional>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

struct MinimalCountdownLoopSlice {
  std::string function_name;
  std::string loop_label;
  std::string body_label;
  std::string exit_label;
  std::int64_t initial_imm = 0;
};

std::string direct_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
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

}  // namespace

std::optional<std::string> try_emit_minimal_countdown_loop_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_countdown_loop_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }

  const auto symbol = direct_symbol_name(module.target_triple, slice->function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << ".text\n"
      << ".globl " << symbol << "\n";
  if (module.target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol << ", @function\n";
  }
  out << symbol << ":\n"
      << "  mov eax, " << slice->initial_imm << "\n"
      << ".L" << slice->loop_label << ":\n"
      << "  cmp eax, 0\n"
      << "  je .L" << slice->exit_label << "\n"
      << "  jmp .L" << slice->body_label << "\n"
      << ".L" << slice->body_label << ":\n"
      << "  sub eax, 1\n"
      << "  jmp .L" << slice->loop_label << "\n"
      << ".L" << slice->exit_label << ":\n"
      << "  ret\n";
  return out.str();
}

}  // namespace c4c::backend::x86
