#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

const c4c::backend::bir::Block* find_block(const c4c::backend::bir::Function& function,
                                           std::string_view label) {
  for (const auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

std::string render_stack_memory_operand(std::size_t byte_offset, std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

[[nodiscard]] bool is_i32_named_value(const c4c::backend::bir::Value& value,
                                      std::string_view expected_name) {
  return value.kind == c4c::backend::bir::Value::Kind::Named &&
         value.type == c4c::backend::bir::TypeKind::I32 && value.name == expected_name;
}

[[nodiscard]] bool is_i32_zero_immediate(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         value.type == c4c::backend::bir::TypeKind::I32 && value.immediate == 0;
}

[[nodiscard]] bool is_i32_immediate(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         value.type == c4c::backend::bir::TypeKind::I32;
}

}  // namespace

std::optional<std::string> render_prepared_loop_join_countdown_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  if (!function.params.empty() ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
      prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }

  const auto* loop_block = static_cast<const c4c::backend::bir::Block*>(nullptr);
  const auto* branch_condition =
      static_cast<const c4c::backend::prepare::PreparedBranchCondition*>(nullptr);
  const auto* join_transfer =
      static_cast<const c4c::backend::prepare::PreparedJoinTransfer*>(nullptr);
  const auto* join_edges =
      static_cast<const std::vector<c4c::backend::prepare::PreparedEdgeValueTransfer>*>(nullptr);
  for (const auto& candidate_join_transfer : function_control_flow.join_transfers) {
    if (candidate_join_transfer.kind != c4c::backend::prepare::PreparedJoinTransferKind::LoopCarry ||
        candidate_join_transfer.result.type != c4c::backend::bir::TypeKind::I32 ||
        candidate_join_transfer.edge_transfers.size() != 2) {
      continue;
    }

    const auto* candidate_loop_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(prepared_names,
                                                    candidate_join_transfer.join_block_label));
    if (candidate_loop_block == nullptr || candidate_loop_block == &entry ||
        candidate_loop_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }
    const auto* candidate_loop_compare =
        candidate_loop_block->insts.empty()
            ? nullptr
            : std::get_if<c4c::backend::bir::BinaryInst>(&candidate_loop_block->insts.back());
    if (candidate_loop_compare == nullptr ||
        candidate_loop_compare->result.kind != c4c::backend::bir::Value::Kind::Named ||
        candidate_loop_compare->result.type != c4c::backend::bir::TypeKind::I32) {
      continue;
    }

    const auto* candidate_branch_condition =
        [&]() -> const c4c::backend::prepare::PreparedBranchCondition* {
          const auto candidate_loop_block_label_id =
              c4c::backend::prepare::resolve_prepared_block_label_id(prepared_names,
                                                                     candidate_loop_block->label);
          if (!candidate_loop_block_label_id.has_value()) {
            return nullptr;
          }
          return c4c::backend::prepare::find_prepared_branch_condition(
              function_control_flow, *candidate_loop_block_label_id);
        }();
    if (candidate_branch_condition == nullptr || !candidate_branch_condition->predicate.has_value() ||
        !candidate_branch_condition->compare_type.has_value() ||
        !candidate_branch_condition->lhs.has_value() ||
        !candidate_branch_condition->rhs.has_value() ||
        *candidate_branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
        candidate_branch_condition->condition_value.kind !=
            c4c::backend::bir::Value::Kind::Named ||
        candidate_branch_condition->condition_value.name != candidate_loop_compare->result.name) {
      continue;
    }

    const auto* carried_counter = [&]() -> const c4c::backend::bir::Value* {
      if (candidate_branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
          candidate_branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          candidate_branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32 &&
          candidate_branch_condition->rhs->immediate == 0) {
        return &*candidate_branch_condition->lhs;
      }
      if (candidate_branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
          candidate_branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          candidate_branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32 &&
          candidate_branch_condition->lhs->immediate == 0) {
        return &*candidate_branch_condition->rhs;
      }
      return static_cast<const c4c::backend::bir::Value*>(nullptr);
    }();
    if (carried_counter == nullptr || carried_counter->name != candidate_join_transfer.result.name) {
      continue;
    }

    const auto carried_counter_id =
        c4c::backend::prepare::resolve_prepared_value_name_id(prepared_names,
                                                              candidate_join_transfer.result.name);
    if (!carried_counter_id.has_value()) {
      continue;
    }
    const auto* candidate_join_edges = c4c::backend::prepare::incoming_transfers_for_join(
        prepared_names,
        function_control_flow,
        candidate_join_transfer.join_block_label,
        *carried_counter_id);
    if (candidate_join_edges == nullptr || candidate_join_edges->size() != 2) {
      continue;
    }

    loop_block = candidate_loop_block;
    branch_condition = candidate_branch_condition;
    join_transfer = &candidate_join_transfer;
    join_edges = candidate_join_edges;
    break;
  }
  if (loop_block == nullptr || branch_condition == nullptr || join_transfer == nullptr ||
      join_edges == nullptr) {
    return std::nullopt;
  }

  const bool lhs_is_counter_rhs_is_zero =
      branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
      branch_condition->lhs->name == join_transfer->result.name &&
      branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
      branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32 &&
      branch_condition->rhs->immediate == 0;
  const bool rhs_is_counter_lhs_is_zero =
      branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
      branch_condition->rhs->name == join_transfer->result.name &&
      branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
      branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32 &&
      branch_condition->lhs->immediate == 0;
  if (!lhs_is_counter_rhs_is_zero && !rhs_is_counter_lhs_is_zero) {
    return std::nullopt;
  }

  std::string body_label;
  std::string exit_label;
  if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne) {
    body_label = std::string(
        c4c::backend::prepare::prepared_block_label(prepared_names, branch_condition->true_label));
    exit_label = std::string(c4c::backend::prepare::prepared_block_label(
        prepared_names, branch_condition->false_label));
  } else if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq) {
    body_label = std::string(c4c::backend::prepare::prepared_block_label(
        prepared_names, branch_condition->false_label));
    exit_label = std::string(
        c4c::backend::prepare::prepared_block_label(prepared_names, branch_condition->true_label));
  } else {
    return std::nullopt;
  }

  const auto* body_block = find_block(function, body_label);
  const auto* exit_block = find_block(function, exit_label);
  const auto resolve_authoritative_branch_target =
      [&](const c4c::backend::bir::Block& source_block) -> const c4c::backend::bir::Block* {
    const auto source_block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(prepared_names,
                                                               source_block.label);
    if (!source_block_label_id.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* prepared_block = c4c::backend::prepare::find_prepared_control_flow_block(
        function_control_flow, *source_block_label_id);
    if (prepared_block == nullptr ||
        prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
        prepared_block->branch_target_label == c4c::kInvalidBlockLabel) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* target_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(prepared_names,
                                                    prepared_block->branch_target_label));
    if (target_block == nullptr || target_block == &source_block) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }
    return target_block;
  };
  if (body_block == nullptr || exit_block == nullptr || body_block == &entry ||
      exit_block == &entry || body_block == loop_block || exit_block == loop_block ||
      exit_block == body_block ||
      loop_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      body_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
      resolve_authoritative_branch_target(*body_block) != loop_block ||
      exit_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !exit_block->terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& exit_value = *exit_block->terminator.value;
  const bool exit_returns_counter =
      exit_value.kind == c4c::backend::bir::Value::Kind::Named &&
      exit_value.type == c4c::backend::bir::TypeKind::I32 &&
      exit_value.name == join_transfer->result.name;
  const bool exit_returns_zero =
      exit_value.kind == c4c::backend::bir::Value::Kind::Immediate &&
      exit_value.type == c4c::backend::bir::TypeKind::I32 && exit_value.immediate == 0;
  if (!exit_returns_counter && !exit_returns_zero) {
    return std::nullopt;
  }

  if (body_block->insts.empty() || loop_block->insts.empty()) {
    return std::nullopt;
  }
  const auto* loop_carrier =
      std::get_if<c4c::backend::bir::LoadLocalInst>(&loop_block->insts.front());
  const auto* body_update =
      std::get_if<c4c::backend::bir::BinaryInst>(&body_block->insts.front());
  if (loop_carrier == nullptr || loop_carrier->result.kind != c4c::backend::bir::Value::Kind::Named ||
      loop_carrier->result.type != c4c::backend::bir::TypeKind::I32 ||
      loop_carrier->result.name != join_transfer->result.name || loop_carrier->byte_offset != 0 ||
      loop_carrier->address.has_value() || body_update == nullptr ||
      body_update->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      body_update->operand_type != c4c::backend::bir::TypeKind::I32 ||
      body_update->result.type != c4c::backend::bir::TypeKind::I32 ||
      body_update->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      body_update->lhs.name != join_transfer->result.name ||
      body_update->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      body_update->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      body_update->rhs.immediate != 1) {
    return std::nullopt;
  }

  const c4c::backend::prepare::PreparedEdgeValueTransfer* init_incoming = nullptr;
  const c4c::backend::prepare::PreparedEdgeValueTransfer* body_incoming = nullptr;
  for (const auto& incoming : *join_edges) {
    if (c4c::backend::prepare::prepared_block_label(prepared_names, incoming.predecessor_label) ==
        body_block->label) {
      body_incoming = &incoming;
    } else {
      init_incoming = &incoming;
    }
  }
  const auto* init_block =
      init_incoming == nullptr
          ? nullptr
          : find_block(function,
                       c4c::backend::prepare::prepared_block_label(prepared_names,
                                                                   init_incoming->predecessor_label));
  const auto block_has_supported_init_handoff_carrier =
      [&](const c4c::backend::bir::Block& candidate) -> bool {
    if (candidate.insts.size() != 1) {
      return false;
    }
    const auto* init_store =
        std::get_if<c4c::backend::bir::StoreLocalInst>(&candidate.insts.front());
    return init_store != nullptr && init_store->byte_offset == 0 &&
           !init_store->address.has_value() &&
           init_store->slot_name == loop_carrier->slot_name;
  };
  const auto find_unique_transparent_branch_predecessor =
      [&](const c4c::backend::bir::Block& target_block) -> const c4c::backend::bir::Block* {
    const c4c::backend::bir::Block* predecessor = nullptr;
    for (const auto& candidate : function.blocks) {
      if (candidate.terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
        continue;
      }
      if (resolve_authoritative_branch_target(candidate) != &target_block) {
        continue;
      }
      if (predecessor != nullptr) {
        return nullptr;
      }
      predecessor = &candidate;
    }
    return predecessor;
  };
  const bool entry_reaches_loop_through_supported_handoff_prefix = [&]() -> bool {
    if (init_block == nullptr) {
      return false;
    }
    if (init_block == &entry) {
      return resolve_authoritative_branch_target(entry) == loop_block &&
             block_has_supported_init_handoff_carrier(entry);
    }
    if (init_block == loop_block || init_block == body_block || init_block == exit_block ||
        init_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        resolve_authoritative_branch_target(*init_block) != loop_block ||
        !block_has_supported_init_handoff_carrier(*init_block)) {
      return false;
    }

    const auto* predecessor = find_unique_transparent_branch_predecessor(*init_block);
    if (predecessor == nullptr || predecessor == init_block) {
      return false;
    }
    std::size_t transparent_prefix_depth = 0;
    while (predecessor != &entry) {
      if (predecessor == loop_block || predecessor == body_block || predecessor == exit_block ||
          predecessor == init_block || !predecessor->insts.empty()) {
        return false;
      }
      predecessor = find_unique_transparent_branch_predecessor(*predecessor);
      if (predecessor == nullptr || ++transparent_prefix_depth > function.blocks.size()) {
        return false;
      }
    }
    return predecessor->insts.empty() &&
           predecessor->terminator.kind == c4c::backend::bir::TerminatorKind::Branch;
  }();
  if (init_incoming == nullptr || body_incoming == nullptr || init_block == nullptr ||
      !entry_reaches_loop_through_supported_handoff_prefix ||
      init_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      init_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
      init_incoming->incoming_value.immediate < 0 ||
      body_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Named ||
      body_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
      body_incoming->incoming_value.name != body_update->result.name) {
    return std::nullopt;
  }

  const auto loop_label = ".L" + function.name + "_" + loop_block->label;
  const auto body_label_asm = ".L" + function.name + "_" + body_block->label;
  const auto exit_label_asm = ".L" + function.name + "_" + exit_block->label;

  auto asm_text = std::string(asm_prefix);
  asm_text += "    mov eax, " +
              std::to_string(static_cast<std::int32_t>(init_incoming->incoming_value.immediate)) +
              "\n";
  asm_text += loop_label + ":\n";
  asm_text += "    test eax, eax\n";
  asm_text += "    je " + exit_label_asm + "\n";
  asm_text += body_label_asm + ":\n";
  asm_text += "    sub eax, 1\n";
  asm_text += "    jmp " + loop_label + "\n";
  asm_text += exit_label_asm + ":\n";
  if (exit_returns_zero) {
    asm_text += "    mov eax, 0\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

std::optional<std::string> render_prepared_countdown_entry_routes_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  if (prepared_names == nullptr || function_control_flow == nullptr) {
    return std::nullopt;
  }

  if (const auto rendered_loop_join = render_prepared_loop_join_countdown_if_supported(
          function, entry, *prepared_names, *function_control_flow, prepared_arch, asm_prefix);
      rendered_loop_join.has_value()) {
    return rendered_loop_join;
  }

  const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  return render_prepared_local_i32_countdown_loop_if_supported(
      function, entry, prepared_names, function_control_flow, prepared_arch, asm_prefix, *layout);
}

std::optional<std::string> render_prepared_local_i32_countdown_loop_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const PreparedModuleLocalSlotLayout& layout) {
  if (!function.params.empty() || function.blocks.size() < 4 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
      prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  if (prepared_names == nullptr || function_control_flow == nullptr) {
    return std::nullopt;
  }

  const auto resolve_slot_return_block =
      [&](std::string_view label, std::string_view slot_name) -> const c4c::backend::bir::Block* {
    const auto* block = find_block(function, label);
    if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !block->terminator.value.has_value() || block->insts.size() != 1) {
      return nullptr;
    }
    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&block->insts.front());
    if (load == nullptr || load->address.has_value() || load->byte_offset != 0 ||
        load->result.type != c4c::backend::bir::TypeKind::I32 || load->slot_name != slot_name ||
        block->terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
        block->terminator.value->type != c4c::backend::bir::TypeKind::I32 ||
        block->terminator.value->name != load->result.name) {
      return nullptr;
    }
    return block;
  };

  const auto resolve_immediate_return_block =
      [&](std::string_view label) -> const c4c::backend::bir::Block* {
    const auto* block = find_block(function, label);
    if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !block->terminator.value.has_value() || !block->insts.empty() ||
        block->terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
        block->terminator.value->type != c4c::backend::bir::TypeKind::I32) {
      return nullptr;
    }
      return block;
  };

  const auto resolve_authoritative_prepared_branch_target =
      [&](const c4c::backend::bir::Block& source_block) -> const c4c::backend::bir::Block* {
    const auto block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names,
                                                               source_block.label);
    if (!block_label_id.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* prepared_block = c4c::backend::prepare::find_prepared_control_flow_block(
        *function_control_flow, *block_label_id);
    if (prepared_block == nullptr) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }
    if (prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
        prepared_block->branch_target_label == c4c::kInvalidBlockLabel) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* target_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(*prepared_names,
                                                    prepared_block->branch_target_label));
    if (target_block == nullptr || target_block == &source_block) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }
    return target_block;
  };

  const auto resolve_countdown_successor_target =
      [&](const c4c::backend::bir::Block& source_block) -> const c4c::backend::bir::Block* {
    return resolve_authoritative_prepared_branch_target(source_block);
  };

  const auto match_counter_init_block =
      [&](const c4c::backend::bir::Block* block,
          std::optional<std::string_view> expected_slot_name)
      -> std::optional<std::pair<const c4c::backend::bir::StoreLocalInst*,
                                 const c4c::backend::bir::Block*>> {
    if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        block->insts.empty()) {
      return std::nullopt;
    }

    const c4c::backend::bir::StoreLocalInst* last_store = nullptr;
    for (const auto& inst : block->insts) {
      const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store == nullptr || store->address.has_value() || store->byte_offset != 0 ||
          store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      if (expected_slot_name.has_value() && store->slot_name != *expected_slot_name) {
        return std::nullopt;
      }
      if (last_store != nullptr && store->slot_name != last_store->slot_name) {
        return std::nullopt;
      }
      last_store = store;
    }

    const auto* next_block = resolve_countdown_successor_target(*block);
    if (last_store == nullptr || next_block == nullptr) {
      return std::nullopt;
    }
    return std::pair{last_store, next_block};
  };

  struct CountdownSegment {
    const c4c::backend::bir::Block* init_block = nullptr;
    const c4c::backend::bir::StoreLocalInst* init_store = nullptr;
    const c4c::backend::bir::Block* entry_target = nullptr;
    const c4c::backend::bir::Block* cond_block = nullptr;
    const c4c::backend::bir::LoadLocalInst* cond_load = nullptr;
    const c4c::backend::bir::BinaryInst* cond_compare = nullptr;
    const c4c::backend::bir::Block* body_block = nullptr;
    const c4c::backend::bir::BinaryInst* body_sub = nullptr;
    const c4c::backend::bir::Block* return_block = nullptr;
    const c4c::backend::bir::Block* guard_block = nullptr;
    const c4c::backend::bir::Block* guard_true_return_block = nullptr;
    const c4c::backend::bir::Block* continuation_init_block = nullptr;
    std::int64_t emitted_init_immediate = 0;
  };

  struct ResolvedBranchTargets {
    const c4c::backend::prepare::PreparedBranchCondition* branch_condition = nullptr;
    const c4c::backend::bir::Block* true_block = nullptr;
    const c4c::backend::bir::Block* false_block = nullptr;
  };

  const auto resolve_control_flow_branch_targets =
      [&](const c4c::backend::bir::Block& block)
      -> std::optional<std::pair<const c4c::backend::bir::Block*,
                                 const c4c::backend::bir::Block*>> {
    if (prepared_names == nullptr || function_control_flow == nullptr) {
      return std::nullopt;
    }

    const auto block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, block.label);
    if (!block_label_id.has_value()) {
      return std::nullopt;
    }

    const auto prepared_target_labels =
        c4c::backend::prepare::find_prepared_control_flow_branch_target_labels(
            *function_control_flow, *block_label_id);
    if (!prepared_target_labels.has_value()) {
      return std::nullopt;
    }

    const auto* true_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(*prepared_names,
                                                    prepared_target_labels->true_label));
    const auto* false_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(*prepared_names,
                                                    prepared_target_labels->false_label));
    if (true_block == nullptr || false_block == nullptr || true_block == &block ||
        false_block == &block) {
      return std::nullopt;
    }

    return std::pair{true_block, false_block};
  };

  const auto resolve_branch_targets =
      [&](const c4c::backend::bir::Block& block,
          std::string_view expected_condition_name,
          std::string_view expected_counter_name) -> std::optional<ResolvedBranchTargets> {
    if (prepared_names == nullptr || function_control_flow == nullptr) {
      return std::nullopt;
    }

    const auto block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, block.label);
    if (!block_label_id.has_value()) {
      return std::nullopt;
    }

    const auto* branch_condition =
        c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                              *block_label_id);
    if (branch_condition == nullptr) {
      if (const auto control_flow_targets = resolve_control_flow_branch_targets(block);
          control_flow_targets.has_value()) {
        return ResolvedBranchTargets{
            .branch_condition = nullptr,
            .true_block = control_flow_targets->first,
            .false_block = control_flow_targets->second,
        };
      }
      return std::nullopt;
    }
    if (!branch_condition->compare_type.has_value() ||
        *branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
        !branch_condition->lhs.has_value() || !branch_condition->rhs.has_value() ||
        branch_condition->condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
        branch_condition->condition_value.name != expected_condition_name ||
        (!is_i32_named_value(*branch_condition->lhs, expected_counter_name) ||
         !is_i32_zero_immediate(*branch_condition->rhs)) &&
            (!is_i32_named_value(*branch_condition->rhs, expected_counter_name) ||
             !is_i32_zero_immediate(*branch_condition->lhs))) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto target_labels = c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
        *prepared_names,
        function_control_flow,
        *block_label_id,
        block,
        *branch_condition);
    if (!target_labels.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* true_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->true_label));
    const auto* false_block = find_block(
        function,
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->false_label));
    if (true_block == nullptr || false_block == nullptr || true_block == &block ||
        false_block == &block) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    return ResolvedBranchTargets{
        .branch_condition = branch_condition,
        .true_block = true_block,
        .false_block = false_block,
    };
  };

  const auto apply_authoritative_join_transfer =
      [&](CountdownSegment& segment) -> const c4c::backend::prepare::PreparedJoinTransfer* {
    if (prepared_names == nullptr || function_control_flow == nullptr || segment.cond_block == nullptr ||
        segment.cond_load == nullptr || segment.body_block == nullptr || segment.body_sub == nullptr) {
      return nullptr;
    }

    const auto join_block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names,
                                                               segment.cond_block->label);
    if (!join_block_label_id.has_value()) {
      return nullptr;
    }

    const auto* join_transfer = c4c::backend::prepare::find_prepared_join_transfer(
        *prepared_names,
        *function_control_flow,
        *join_block_label_id,
        segment.cond_load->result.name);
    if (join_transfer == nullptr) {
      return nullptr;
    }

    if (join_transfer->kind != c4c::backend::prepare::PreparedJoinTransferKind::EdgeStoreSlot ||
        join_transfer->edge_transfers.size() != 2 || !join_transfer->storage_name.has_value() ||
        c4c::backend::prepare::prepared_slot_name(*prepared_names, *join_transfer->storage_name) !=
            segment.init_store->slot_name) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    const auto* init_incoming = static_cast<const c4c::backend::prepare::PreparedEdgeValueTransfer*>(nullptr);
    const auto* body_incoming = static_cast<const c4c::backend::prepare::PreparedEdgeValueTransfer*>(nullptr);
    for (const auto& incoming : join_transfer->edge_transfers) {
      const auto predecessor_label =
          c4c::backend::prepare::prepared_block_label(*prepared_names, incoming.predecessor_label);
      const auto successor_label =
          c4c::backend::prepare::prepared_block_label(*prepared_names, incoming.successor_label);
      if (successor_label != segment.cond_block->label) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
      }
      if (predecessor_label == segment.init_block->label) {
        init_incoming = &incoming;
      } else if (predecessor_label == segment.body_block->label) {
        body_incoming = &incoming;
      } else {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
      }
    }

    if (init_incoming == nullptr || body_incoming == nullptr ||
        !is_i32_immediate(init_incoming->incoming_value) ||
        !is_i32_named_value(body_incoming->incoming_value, segment.body_sub->result.name) ||
        !is_i32_named_value(join_transfer->result, segment.cond_load->result.name)) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
    }

    segment.emitted_init_immediate = init_incoming->incoming_value.immediate;
    return join_transfer;
  };

  const auto match_countdown_segment =
      [&](const c4c::backend::bir::Block* init_block,
          std::optional<std::string_view> expected_slot_name,
          bool allow_guard_exit) -> std::optional<CountdownSegment> {
    const auto init_info = match_counter_init_block(init_block, expected_slot_name);
    if (!init_info.has_value()) {
      return std::nullopt;
    }

    CountdownSegment segment;
    segment.init_block = init_block;
    segment.init_store = init_info->first;
    segment.entry_target = init_info->second;
    segment.emitted_init_immediate = segment.init_store->value.immediate;
    if (layout.offsets.find(segment.init_store->slot_name) == layout.offsets.end() ||
        segment.entry_target == nullptr || segment.entry_target == init_block) {
      return std::nullopt;
    }

    const auto match_cond_block =
        [&](const c4c::backend::bir::Block* cond_block)
        -> std::optional<std::pair<const c4c::backend::bir::LoadLocalInst*,
                                   const c4c::backend::bir::BinaryInst*>> {
      if (cond_block == nullptr || cond_block == init_block || cond_block->insts.size() != 2 ||
          cond_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
        return std::nullopt;
      }
      const auto* cond_load =
          std::get_if<c4c::backend::bir::LoadLocalInst>(&cond_block->insts.front());
      const auto* cond_compare =
          std::get_if<c4c::backend::bir::BinaryInst>(&cond_block->insts.back());
      if (cond_load == nullptr || cond_compare == nullptr || cond_load->address.has_value() ||
          cond_load->byte_offset != 0 ||
          cond_load->result.type != c4c::backend::bir::TypeKind::I32 ||
          cond_load->slot_name != segment.init_store->slot_name ||
          cond_compare->opcode != c4c::backend::bir::BinaryOpcode::Ne ||
          cond_compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
          (cond_compare->result.type != c4c::backend::bir::TypeKind::I1 &&
           cond_compare->result.type != c4c::backend::bir::TypeKind::I32) ||
          cond_compare->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
          cond_compare->lhs.name != cond_load->result.name ||
          cond_compare->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
          cond_compare->rhs.type != c4c::backend::bir::TypeKind::I32 ||
          cond_compare->rhs.immediate != 0) {
        return std::nullopt;
      }
      return std::pair{cond_load, cond_compare};
    };

    const auto match_body_block =
        [&](const c4c::backend::bir::Block* body_block,
            const c4c::backend::bir::Block* cond_block)
        -> std::optional<const c4c::backend::bir::BinaryInst*> {
      if (body_block == nullptr || body_block == init_block || body_block == cond_block ||
          body_block->insts.size() != 3 ||
          body_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
        return std::nullopt;
      }
      const auto* body_load =
          std::get_if<c4c::backend::bir::LoadLocalInst>(&body_block->insts[0]);
      const auto* body_sub =
          std::get_if<c4c::backend::bir::BinaryInst>(&body_block->insts[1]);
      const auto* body_store =
          std::get_if<c4c::backend::bir::StoreLocalInst>(&body_block->insts[2]);
      if (body_load == nullptr || body_sub == nullptr || body_store == nullptr ||
          body_load->address.has_value() || body_load->byte_offset != 0 ||
          body_load->result.type != c4c::backend::bir::TypeKind::I32 ||
          body_load->slot_name != segment.init_store->slot_name ||
          body_sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
          body_sub->operand_type != c4c::backend::bir::TypeKind::I32 ||
          body_sub->result.type != c4c::backend::bir::TypeKind::I32 ||
          body_sub->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
          body_sub->lhs.name != body_load->result.name ||
          body_sub->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
          body_sub->rhs.type != c4c::backend::bir::TypeKind::I32 ||
          body_sub->rhs.immediate != 1 || body_store->address.has_value() ||
          body_store->byte_offset != 0 ||
          body_store->slot_name != segment.init_store->slot_name ||
          body_store->value.kind != c4c::backend::bir::Value::Kind::Named ||
          body_store->value.name != body_sub->result.name ||
          body_store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto* next_block = resolve_countdown_successor_target(*body_block);
      if (next_block != cond_block) {
        return std::nullopt;
      }
      return body_sub;
    };

    const auto resolve_cond_targets =
        [&](const c4c::backend::bir::Block& cond_block,
            const c4c::backend::bir::LoadLocalInst& cond_load,
            const c4c::backend::bir::BinaryInst& cond_compare)
        -> std::optional<std::pair<const c4c::backend::bir::Block*,
                                   const c4c::backend::bir::Block*>> {
      if (const auto authoritative_targets =
              resolve_branch_targets(cond_block, cond_compare.result.name, cond_load.result.name);
          authoritative_targets.has_value()) {
        return std::pair{authoritative_targets->true_block, authoritative_targets->false_block};
      }
      const auto* true_block = find_block(function, cond_block.terminator.true_label);
      const auto* false_block = find_block(function, cond_block.terminator.false_label);
      if (true_block == nullptr || false_block == nullptr || true_block == &cond_block ||
          false_block == &cond_block) {
        return std::nullopt;
      }
      return std::pair{true_block, false_block};
    };

    if (const auto cond_match = match_cond_block(segment.entry_target); cond_match.has_value()) {
      const auto cond_targets = resolve_cond_targets(*segment.entry_target,
                                                    *cond_match->first,
                                                    *cond_match->second);
      if (cond_targets.has_value()) {
        const auto* candidate_body =
            match_body_block(cond_targets->first, segment.entry_target).has_value()
                ? cond_targets->first
                : cond_targets->second;
        if (const auto body_match = match_body_block(candidate_body, segment.entry_target);
            body_match.has_value()) {
          const auto* candidate_false_target =
              candidate_body == cond_targets->first ? cond_targets->second : cond_targets->first;
          segment.cond_block = segment.entry_target;
          segment.cond_load = cond_match->first;
          segment.cond_compare = cond_match->second;
          segment.body_block = candidate_body;
          segment.body_sub = *body_match;
          segment.return_block = candidate_false_target;
        }
      }
    }

    if (segment.cond_block == nullptr &&
        segment.entry_target->terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
      const auto* candidate_cond = resolve_countdown_successor_target(*segment.entry_target);
      if (const auto cond_match = match_cond_block(candidate_cond); cond_match.has_value()) {
        const auto cond_targets =
            resolve_cond_targets(*candidate_cond, *cond_match->first, *cond_match->second);
        if (cond_targets.has_value()) {
          const auto* candidate_body =
              cond_targets->first == segment.entry_target ? cond_targets->first
                                                          : cond_targets->second;
          if (candidate_body == segment.entry_target &&
              match_body_block(candidate_body, candidate_cond).has_value()) {
            const auto* candidate_false_target =
                candidate_body == cond_targets->first ? cond_targets->second : cond_targets->first;
            segment.cond_block = candidate_cond;
            segment.cond_load = cond_match->first;
            segment.cond_compare = cond_match->second;
            segment.body_block = segment.entry_target;
            segment.body_sub = *match_body_block(candidate_body, candidate_cond);
            segment.return_block = candidate_false_target;
          }
        }
      }
    }

    if (segment.cond_block == nullptr || segment.cond_load == nullptr || segment.cond_compare == nullptr ||
        segment.body_block == nullptr || segment.body_sub == nullptr || segment.return_block == nullptr) {
      return std::nullopt;
    }

    if (const auto* return_block =
            resolve_slot_return_block(segment.return_block->label, segment.init_store->slot_name);
        return_block != nullptr) {
      segment.return_block = return_block;
      apply_authoritative_join_transfer(segment);
      return segment;
    }

    if (!allow_guard_exit) {
      return std::nullopt;
    }

    const auto* guard_block = segment.return_block;
    if (guard_block == nullptr || guard_block->insts.size() != 2 ||
        guard_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto* guard_load =
        std::get_if<c4c::backend::bir::LoadLocalInst>(&guard_block->insts.front());
    const auto* guard_compare =
        std::get_if<c4c::backend::bir::BinaryInst>(&guard_block->insts.back());
    if (guard_load == nullptr || guard_compare == nullptr || guard_load->address.has_value() ||
        guard_load->byte_offset != 0 ||
        guard_load->result.type != c4c::backend::bir::TypeKind::I32 ||
        guard_load->slot_name != segment.init_store->slot_name ||
        guard_compare->opcode != c4c::backend::bir::BinaryOpcode::Ne ||
        guard_compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
        (guard_compare->result.type != c4c::backend::bir::TypeKind::I1 &&
         guard_compare->result.type != c4c::backend::bir::TypeKind::I32) ||
        guard_compare->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
        guard_compare->lhs.name != guard_load->result.name ||
        guard_compare->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
        guard_compare->rhs.type != c4c::backend::bir::TypeKind::I32 ||
        guard_compare->rhs.immediate != 0) {
      return std::nullopt;
    }

    const auto guard_targets =
        [&]() -> std::optional<std::pair<const c4c::backend::bir::Block*,
                                         const c4c::backend::bir::Block*>> {
      if (const auto authoritative_targets =
              resolve_branch_targets(*guard_block, guard_compare->result.name, guard_load->result.name);
          authoritative_targets.has_value()) {
        return std::pair{authoritative_targets->true_block, authoritative_targets->false_block};
      }
      const auto* true_block = find_block(function, guard_block->terminator.true_label);
      const auto* false_block = find_block(function, guard_block->terminator.false_label);
      if (true_block == nullptr || false_block == nullptr || true_block == guard_block ||
          false_block == guard_block) {
        return std::nullopt;
      }
      return std::pair{true_block, false_block};
    }();
    if (!guard_targets.has_value()) {
      return std::nullopt;
    }

    const auto* guard_true_return = resolve_immediate_return_block(guard_targets->first->label);
    const auto* continuation_init = guard_targets->second;
    if (guard_true_return == nullptr || continuation_init == nullptr || continuation_init == guard_block) {
      if (const auto* alternate_return = resolve_immediate_return_block(guard_targets->second->label);
          alternate_return != nullptr && guard_targets->first != nullptr &&
          guard_targets->first != guard_block) {
        guard_true_return = alternate_return;
        continuation_init = guard_targets->first;
      } else {
        return std::nullopt;
      }
    }

    segment.guard_block = guard_block;
    segment.guard_true_return_block = guard_true_return;
    segment.continuation_init_block = continuation_init;
    apply_authoritative_join_transfer(segment);
    return segment;
  };

  const auto first_segment = match_countdown_segment(&entry, std::nullopt, true);
  if (!first_segment.has_value()) {
    return std::nullopt;
  }

  std::optional<CountdownSegment> second_segment;
  if (first_segment->continuation_init_block != nullptr) {
    second_segment = match_countdown_segment(first_segment->continuation_init_block,
                                            std::string_view(first_segment->init_store->slot_name),
                                            false);
    if (!second_segment.has_value() || second_segment->continuation_init_block != nullptr) {
      return std::nullopt;
    }
  } else if (first_segment->return_block == nullptr) {
    return std::nullopt;
  }

  const auto slot_it = layout.offsets.find(first_segment->init_store->slot_name);
  if (slot_it == layout.offsets.end()) {
    return std::nullopt;
  }

  std::unordered_set<const c4c::backend::bir::Block*> used_blocks = {
      first_segment->init_block,
      first_segment->cond_block,
      first_segment->body_block,
  };
  if (first_segment->return_block != nullptr) {
    used_blocks.insert(first_segment->return_block);
  }
  if (first_segment->guard_block != nullptr) {
    used_blocks.insert(first_segment->guard_block);
  }
  if (first_segment->guard_true_return_block != nullptr) {
    used_blocks.insert(first_segment->guard_true_return_block);
  }
  if (first_segment->continuation_init_block != nullptr) {
    used_blocks.insert(first_segment->continuation_init_block);
  }
  if (second_segment.has_value()) {
    used_blocks.insert(second_segment->init_block);
    used_blocks.insert(second_segment->cond_block);
    used_blocks.insert(second_segment->body_block);
    if (second_segment->return_block != nullptr) {
      used_blocks.insert(second_segment->return_block);
    }
  }

  if (function_control_flow != nullptr) {
    const auto join_transfer_references_used_block =
        [&](const c4c::backend::prepare::PreparedJoinTransfer& join_transfer) {
      const auto block_is_used = [&](std::string_view label) {
        const auto* block = find_block(function, label);
        return block != nullptr && used_blocks.find(block) != used_blocks.end();
      };

      if (prepared_names == nullptr) {
        return false;
      }
      if (block_is_used(c4c::backend::prepare::prepared_block_label(
              *prepared_names, join_transfer.join_block_label))) {
        return true;
      }
      if (join_transfer.source_branch_block_label.has_value() &&
          block_is_used(c4c::backend::prepare::prepared_block_label(
              *prepared_names, *join_transfer.source_branch_block_label))) {
        return true;
      }
      for (const auto& incoming : join_transfer.incomings) {
        if (block_is_used(incoming.label)) {
          return true;
        }
      }
      for (const auto& edge_transfer : join_transfer.edge_transfers) {
        if (block_is_used(c4c::backend::prepare::prepared_block_label(
                *prepared_names, edge_transfer.predecessor_label)) ||
            block_is_used(c4c::backend::prepare::prepared_block_label(
                *prepared_names, edge_transfer.successor_label))) {
          return true;
        }
      }
      return false;
    };

    const auto* accepted_second_segment_join_transfer =
        second_segment.has_value() ? apply_authoritative_join_transfer(*second_segment) : nullptr;
    for (const auto& join_transfer : function_control_flow->join_transfers) {
      if (join_transfer.kind == c4c::backend::prepare::PreparedJoinTransferKind::LoopCarry ||
          (&join_transfer != accepted_second_segment_join_transfer &&
           join_transfer_references_used_block(join_transfer))) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared loop-countdown handoff through the canonical prepared-module handoff");
      }
    }
  }

  for (const auto& block : function.blocks) {
    if (used_blocks.find(&block) == used_blocks.end()) {
      return std::nullopt;
    }
  }

  const auto counter_memory = render_stack_memory_operand(slot_it->second, "DWORD");
  const auto render_segment =
      [&](const CountdownSegment& segment,
          const CountdownSegment* next_segment,
          bool emit_return,
          bool emit_init) -> std::string {
    const auto cond_label = ".L" + function.name + "_" + std::string(segment.cond_block->label);
    const auto body_label = ".L" + function.name + "_" + std::string(segment.body_block->label);
    std::string asm_text;
    if (emit_init) {
      asm_text += "    mov " + counter_memory + ", " +
                  std::to_string(static_cast<std::int32_t>(segment.emitted_init_immediate)) +
                  "\n";
      asm_text += "    jmp " +
                  (segment.entry_target == segment.body_block ? body_label : cond_label) + "\n";
    }
    asm_text += cond_label + ":\n";
    asm_text += "    mov eax, " + counter_memory + "\n";
    asm_text += "    test eax, eax\n";
    asm_text += "    jne " + body_label + "\n";
    if (next_segment != nullptr) {
      const auto next_cond_label =
          ".L" + function.name + "_" + std::string(next_segment->cond_block->label);
      const auto next_body_label =
          ".L" + function.name + "_" + std::string(next_segment->body_block->label);
      asm_text += "    mov " + counter_memory + ", " +
                  std::to_string(static_cast<std::int32_t>(next_segment->emitted_init_immediate)) +
                  "\n";
      asm_text += "    jmp " +
                  (next_segment->entry_target == next_segment->body_block ? next_body_label
                                                                          : next_cond_label) +
                  "\n";
    } else if (emit_return) {
      asm_text += "    mov eax, " + counter_memory + "\n";
      if (layout.frame_size != 0) {
        asm_text += "    add rsp, " + std::to_string(layout.frame_size) + "\n";
      }
      asm_text += "    ret\n";
    } else {
      return std::string();
    }
    asm_text += body_label + ":\n";
    asm_text += "    mov eax, " + counter_memory + "\n";
    asm_text += "    sub eax, 1\n";
    asm_text += "    mov " + counter_memory + ", eax\n";
    asm_text += "    jmp " + cond_label + "\n";
    return asm_text;
  };

  auto asm_text = std::string(asm_prefix);
  if (layout.frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout.frame_size) + "\n";
  }
  asm_text += render_segment(*first_segment,
                             second_segment.has_value() ? &*second_segment : nullptr,
                             !second_segment.has_value(),
                             true);
  if (second_segment.has_value()) {
    asm_text += render_segment(*second_segment, nullptr, true, false);
  }
  return asm_text;
}

}  // namespace c4c::backend::x86
