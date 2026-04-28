#include "module.hpp"

#include "../x86.hpp"
#include "../abi/abi.hpp"
#include "../core/core.hpp"

#include "../../../prealloc/prepared_printer.hpp"
#include "../../../prealloc/target_register_profile.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace c4c::backend::x86::module {

namespace {

[[noreturn]] void throw_prepared_control_flow_handoff_error(std::string detail) {
  throw std::invalid_argument("canonical prepared-module handoff rejected x86 control-flow "
                              "label authority: " +
                              std::move(detail));
}

[[noreturn]] void throw_prepared_value_location_handoff_error(std::string detail) {
  throw std::invalid_argument("canonical prepared-module handoff rejected x86 value-location "
                              "authority: " +
                              std::move(detail));
}

[[noreturn]] void throw_unsupported_x86_scalar_handoff_shape() {
  throw std::invalid_argument(
      "x86 prepared-module consumer only supports a minimal single-block i32 return terminator, "
      "a bounded equality-against-immediate guard family with immediate return leaves including "
      "fixed-offset same-module global i32 loads and pointer-backed same-module global roots, or "
      "one bounded compare-against-zero branch family through the canonical prepared-module handoff");
}

std::string render_prepared_label_id(c4c::BlockLabelId label) {
  if (label == c4c::kInvalidBlockLabel) {
    return "<invalid>";
  }
  return "#" + std::to_string(label);
}

std::string render_stack_memory_operand(std::size_t byte_offset, std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string narrow_i16_register_name(std::string_view wide_register) {
  if (wide_register == "rax" || wide_register == "eax") return "ax";
  if (wide_register == "rbx" || wide_register == "ebx") return "bx";
  if (wide_register == "rcx" || wide_register == "ecx") return "cx";
  if (wide_register == "rdx" || wide_register == "edx") return "dx";
  if (wide_register == "rdi" || wide_register == "edi") return "di";
  if (wide_register == "rsi" || wide_register == "esi") return "si";
  if (wide_register == "rbp" || wide_register == "ebp") return "bp";
  if (wide_register == "rsp" || wide_register == "esp") return "sp";
  if (wide_register == "r8" || wide_register == "r8d") return "r8w";
  if (wide_register == "r9" || wide_register == "r9d") return "r9w";
  if (wide_register == "r10" || wide_register == "r10d") return "r10w";
  if (wide_register == "r11" || wide_register == "r11d") return "r11w";
  if (wide_register == "r12" || wide_register == "r12d") return "r12w";
  if (wide_register == "r13" || wide_register == "r13d") return "r13w";
  if (wide_register == "r14" || wide_register == "r14d") return "r14w";
  if (wide_register == "r15" || wide_register == "r15d") return "r15w";
  return std::string(wide_register);
}

std::string narrow_i8_register_name(std::string_view wide_register) {
  if (wide_register == "rax" || wide_register == "eax") return "al";
  if (wide_register == "rbx" || wide_register == "ebx") return "bl";
  if (wide_register == "rcx" || wide_register == "ecx") return "cl";
  if (wide_register == "rdx" || wide_register == "edx") return "dl";
  if (wide_register == "rdi" || wide_register == "edi") return "dil";
  if (wide_register == "rsi" || wide_register == "esi") return "sil";
  if (wide_register == "rbp" || wide_register == "ebp") return "bpl";
  if (wide_register == "rsp" || wide_register == "esp") return "spl";
  if (wide_register == "r8" || wide_register == "r8d") return "r8b";
  if (wide_register == "r9" || wide_register == "r9d") return "r9b";
  if (wide_register == "r10" || wide_register == "r10d") return "r10b";
  if (wide_register == "r11" || wide_register == "r11d") return "r11b";
  if (wide_register == "r12" || wide_register == "r12d") return "r12b";
  if (wide_register == "r13" || wide_register == "r13d") return "r13b";
  if (wide_register == "r14" || wide_register == "r14d") return "r14b";
  if (wide_register == "r15" || wide_register == "r15d") return "r15b";
  return std::string(wide_register);
}

std::size_t normalize_x86_local_frame_adjust(std::size_t prepared_frame_size_bytes) {
  if (prepared_frame_size_bytes == 0) {
    return 0;
  }
  constexpr std::size_t kLocalFrameAlignmentBytes = 16;
  const auto minimum_frame_size =
      std::max(prepared_frame_size_bytes, kLocalFrameAlignmentBytes);
  const auto remainder = minimum_frame_size % kLocalFrameAlignmentBytes;
  return remainder == 0 ? minimum_frame_size
                        : minimum_frame_size + (kLocalFrameAlignmentBytes - remainder);
}

const c4c::backend::prepare::PreparedFrameSlot* find_prepared_frame_slot_by_id(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    c4c::backend::prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

std::string render_global_i32_memory_operand(const Data& data,
                                             std::string_view global_name,
                                             std::size_t byte_offset) {
  std::string operand = "DWORD PTR [rip + " + data.render_asm_symbol_name(global_name);
  if (byte_offset != 0) {
    operand += " + " + std::to_string(byte_offset);
  }
  operand += "]";
  return operand;
}

std::string render_global_ptr_memory_operand(const Data& data, std::string_view global_name) {
  return "QWORD PTR [rip + " + data.render_asm_symbol_name(global_name) + "]";
}

std::string render_function_local_label(std::string_view function_name, std::string_view label) {
  return ".L" + std::string(function_name) + "_" + std::string(label);
}

std::string escape_asm_string(std::string_view raw_bytes) {
  std::string escaped;
  escaped.reserve(raw_bytes.size());
  for (const unsigned char ch : raw_bytes) {
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
      default:
        escaped.push_back(static_cast<char>(ch));
        break;
    }
  }
  return escaped;
}

std::string_view require_prepared_block_label(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId label,
    std::string_view context) {
  if (label == c4c::kInvalidBlockLabel) {
    throw_prepared_control_flow_handoff_error(std::string(context) + " has an invalid label id");
  }
  const auto spelling = c4c::backend::prepare::prepared_block_label(names, label);
  if (spelling.empty()) {
    throw_prepared_control_flow_handoff_error(std::string(context) + " references unknown label " +
                                              render_prepared_label_id(label));
  }
  return spelling;
}

bool bir_block_matches_prepared_label(
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  const auto prepared_spelling = c4c::backend::prepare::prepared_block_label(names, prepared_label);
  if (block.label_id != c4c::kInvalidBlockLabel) {
    return bir_names.block_labels.spelling(block.label_id) == prepared_spelling;
  }
  return block.label == prepared_spelling;
}

bool bir_block_id_matches_prepared_label(
    const c4c::backend::bir::Block& block,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  if (block.label_id == c4c::kInvalidBlockLabel) {
    return false;
  }
  const auto prepared_spelling = c4c::backend::prepare::prepared_block_label(names, prepared_label);
  return bir_names.block_labels.spelling(block.label_id) == prepared_spelling;
}

const c4c::backend::bir::Block* find_bir_block_by_prepared_label(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  for (const auto& block : function.blocks) {
    if (bir_block_matches_prepared_label(block, bir_names, names, prepared_label)) {
      return &block;
    }
  }
  return nullptr;
}

std::optional<std::size_t> find_bir_block_index_by_prepared_label_id(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    if (bir_block_id_matches_prepared_label(
            function.blocks[block_index], bir_names, names, prepared_label)) {
      return block_index;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> find_bir_block_index_by_pointer(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& target_block) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    if (&function.blocks[block_index] == &target_block) {
      return block_index;
    }
  }
  return std::nullopt;
}

bool prepared_compare_join_owns_branch_target_block(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& candidate_block,
    c4c::BlockLabelId candidate_label);

bool prepared_compare_join_owns_branch_target_label(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId candidate_label) {
  const auto* candidate_block =
      find_bir_block_by_prepared_label(function, bir_names, mutable_names, candidate_label);
  return candidate_block != nullptr &&
         prepared_compare_join_owns_branch_target_block(
             mutable_names, bir_names, control_flow, function, *candidate_block, candidate_label);
}

bool prepared_compare_join_metadata_owns_target_label(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId source_block_label,
    c4c::BlockLabelId target_label) {
  const auto continuation_targets =
      c4c::backend::prepare::find_prepared_compare_join_continuation_targets(
          mutable_names, control_flow, function, source_block_label);
  if (!continuation_targets.has_value() ||
      (continuation_targets->true_label != target_label &&
       continuation_targets->false_label != target_label)) {
    const auto* prepared_block =
        c4c::backend::prepare::find_prepared_control_flow_block(control_flow, source_block_label);
    if (prepared_block == nullptr ||
        prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        (prepared_block->true_label != target_label &&
         prepared_block->false_label != target_label)) {
      return false;
    }

    const auto authoritative_join_transfer =
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            mutable_names, control_flow, source_block_label);
    if (!authoritative_join_transfer.has_value() ||
        authoritative_join_transfer->join_transfer == nullptr ||
        authoritative_join_transfer->true_transfer == nullptr ||
        authoritative_join_transfer->false_transfer == nullptr ||
        c4c::backend::prepare::effective_prepared_join_transfer_carrier_kind(
            *authoritative_join_transfer->join_transfer) ==
            c4c::backend::prepare::PreparedJoinTransferCarrierKind::None ||
        c4c::backend::prepare::find_prepared_control_flow_block(
            control_flow, authoritative_join_transfer->join_transfer->join_block_label) ==
            nullptr) {
      return false;
    }
  }

  const auto* branch_condition =
      c4c::backend::prepare::find_prepared_branch_condition(control_flow, source_block_label);
  if (branch_condition == nullptr) {
    return false;
  }
  return prepared_compare_join_owns_branch_target_label(mutable_names,
                                                        bir_names,
                                                        control_flow,
                                                        function,
                                                        branch_condition->true_label) &&
         prepared_compare_join_owns_branch_target_label(mutable_names,
                                                        bir_names,
                                                        control_flow,
                                                        function,
                                                        branch_condition->false_label);
}

void require_prepared_target_block(const c4c::backend::bir::Function& function,
                                   const c4c::backend::bir::NameTables& bir_names,
                                   c4c::backend::prepare::PreparedNameTables& mutable_names,
                                   c4c::BlockLabelId target_label,
                                   std::string_view context,
                                   const c4c::backend::prepare::PreparedControlFlowFunction*
                                       control_flow = nullptr,
                                   c4c::BlockLabelId source_block_label =
                                       c4c::kInvalidBlockLabel) {
  require_prepared_block_label(mutable_names, target_label, context);
  if (find_bir_block_by_prepared_label(function, bir_names, mutable_names, target_label) ==
      nullptr) {
    if (control_flow != nullptr &&
        prepared_compare_join_metadata_owns_target_label(mutable_names,
                                                         bir_names,
                                                         *control_flow,
                                                         function,
                                                         source_block_label,
                                                         target_label)) {
      return;
    }
    throw_prepared_control_flow_handoff_error(std::string(context) + " label " +
                                              render_prepared_label_id(target_label) +
                                              " does not name a BIR block by id");
  }
}

std::optional<c4c::BlockLabelId> bir_block_label_id(
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Block& block) {
  if (block.label_id != c4c::kInvalidBlockLabel) {
    const auto structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      return c4c::backend::prepare::resolve_prepared_block_label_id(names, structured_label);
    }
  }
  return c4c::backend::prepare::resolve_prepared_block_label_id(names, block.label);
}

void validate_prepared_block_targets(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedControlFlowBlock& prepared_block) {
  const auto* bir_block = find_bir_block_by_prepared_label(
      function, bir_names, mutable_names, prepared_block.block_label);
  if (bir_block == nullptr) {
    throw_prepared_control_flow_handoff_error(
        "prepared block " + render_prepared_label_id(prepared_block.block_label) +
        " does not match any BIR block by label id");
  }
  if (bir_block->terminator.kind != prepared_block.terminator_kind) {
    throw_prepared_control_flow_handoff_error(
        "prepared block " + render_prepared_label_id(prepared_block.block_label) +
        " terminator kind drifted from BIR");
  }

  switch (prepared_block.terminator_kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      break;
    case c4c::backend::bir::TerminatorKind::Branch:
      require_prepared_target_block(function,
                                    bir_names,
                                    mutable_names,
                                    prepared_block.branch_target_label,
                                    "prepared branch target",
                                    &control_flow,
                                    prepared_block.block_label);
      break;
    case c4c::backend::bir::TerminatorKind::CondBranch:
      require_prepared_target_block(function,
                                    bir_names,
                                    mutable_names,
                                    prepared_block.true_label,
                                    "prepared true branch target",
                                    &control_flow,
                                    prepared_block.block_label);
      require_prepared_target_block(function,
                                    bir_names,
                                    mutable_names,
                                    prepared_block.false_label,
                                    "prepared false branch target",
                                    &control_flow,
                                    prepared_block.block_label);
      break;
  }
}

bool prepared_branch_condition_matches_block_or_join_metadata(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedControlFlowBlock& block,
    const c4c::backend::prepare::PreparedBranchCondition& condition) {
  if (block.terminator_kind != c4c::backend::bir::TerminatorKind::CondBranch) {
    return false;
  }
  if (block.true_label == condition.true_label && block.false_label == condition.false_label) {
    return true;
  }

  const auto continuation_targets =
      c4c::backend::prepare::find_prepared_compare_join_continuation_targets(
          mutable_names, control_flow, function, condition.block_label);
  return continuation_targets.has_value() &&
         continuation_targets->true_label == block.true_label &&
         continuation_targets->false_label == block.false_label &&
         prepared_compare_join_owns_branch_target_label(mutable_names,
                                                        bir_names,
                                                        control_flow,
                                                        function,
                                                        condition.true_label) &&
         prepared_compare_join_owns_branch_target_label(mutable_names,
                                                        bir_names,
                                                        control_flow,
                                                        function,
                                                        condition.false_label);
}

bool prepared_short_circuit_branch_plan_owns_condition_targets(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedBranchCondition& condition) {
  for (const auto& source_block : function.blocks) {
    if (source_block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }

    const auto source_label =
        bir_block_label_id(bir_names, mutable_names, source_block);
    if (!source_label.has_value()) {
      continue;
    }

    const auto join_context =
        c4c::backend::prepare::find_prepared_short_circuit_join_context(
            mutable_names, control_flow, function, *source_label);
    if (!join_context.has_value()) {
      continue;
    }

    const auto* source_condition =
        c4c::backend::prepare::find_prepared_branch_condition(control_flow, *source_label);
    if (source_condition == nullptr) {
      continue;
    }

    const auto direct_targets =
        c4c::backend::prepare::find_prepared_compare_branch_target_labels(
            mutable_names, *source_condition, source_block);
    if (!direct_targets.has_value()) {
      continue;
    }

    const auto branch_plan =
        c4c::backend::prepare::find_prepared_short_circuit_branch_plan(
            mutable_names, *join_context, *direct_targets);
    if (!branch_plan.has_value()) {
      continue;
    }

    const auto target_owns_condition =
        [&](const c4c::backend::prepare::PreparedShortCircuitTargetLabels& target) {
          if (target.block_label != condition.block_label || !target.continuation.has_value()) {
            return false;
          }
          if (join_context->join_transfer != nullptr &&
              join_context->join_transfer->continuation_true_label.has_value() &&
              join_context->join_transfer->continuation_false_label.has_value() &&
              (*join_context->join_transfer->continuation_true_label != condition.true_label ||
               *join_context->join_transfer->continuation_false_label != condition.false_label)) {
            return false;
          }
          return target.continuation->true_label == condition.true_label &&
                 target.continuation->false_label == condition.false_label;
        };
    if (target_owns_condition(branch_plan->on_compare_true) ||
        target_owns_condition(branch_plan->on_compare_false)) {
      return true;
    }
  }

  return false;
}

void validate_prepared_branch_condition(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedBranchCondition& condition) {
  require_prepared_target_block(
      function, bir_names, mutable_names, condition.block_label, "prepared condition block");
  require_prepared_target_block(
      function,
      bir_names,
      mutable_names,
      condition.true_label,
      "prepared condition true target",
      &control_flow,
      condition.block_label);
  require_prepared_target_block(
      function,
      bir_names,
      mutable_names,
      condition.false_label,
      "prepared condition false target",
      &control_flow,
      condition.block_label);

  const bool has_any_compare_authority =
      condition.predicate.has_value() || condition.compare_type.has_value() ||
      condition.lhs.has_value() || condition.rhs.has_value();
  const bool requires_complete_compare_authority =
      condition.kind == c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare ||
      condition.can_fuse_with_branch || has_any_compare_authority;
  if (requires_complete_compare_authority &&
      (!condition.predicate.has_value() || !condition.compare_type.has_value() ||
       !condition.lhs.has_value() || !condition.rhs.has_value())) {
    throw_prepared_control_flow_handoff_error(
        "prepared branch condition compare authority is incomplete");
  }

  const auto* block = c4c::backend::prepare::find_prepared_control_flow_block(
      control_flow, condition.block_label);
  if (block == nullptr ||
      !prepared_branch_condition_matches_block_or_join_metadata(
          mutable_names, bir_names, control_flow, function, *block, condition)) {
    if (prepared_short_circuit_branch_plan_owns_condition_targets(
            mutable_names, bir_names, control_flow, function, condition)) {
      return;
    }
    throw_prepared_control_flow_handoff_error(
        "prepared branch condition targets drifted from prepared block targets");
  }
}

bool prepared_compare_join_owns_predecessor_block(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& candidate_block) {
  if (function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto param_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(mutable_names,
                                                            function.params.front().name);
  if (!param_name.has_value()) {
    return false;
  }

  for (const auto& source_block : function.blocks) {
    if (source_block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }

    const auto source_block_label =
        bir_block_label_id(bir_names, mutable_names, source_block);
    if (!source_block_label.has_value()) {
      continue;
    }

    const auto compare_join_context =
        c4c::backend::prepare::find_prepared_param_zero_materialized_compare_join_context(
            mutable_names,
            control_flow,
            function,
            *source_block_label,
            source_block,
            *param_name,
            false);
    if (!compare_join_context.has_value() ||
        compare_join_context->compare_join_context.join_transfer == nullptr ||
        compare_join_context->compare_join_context.true_transfer == nullptr ||
        compare_join_context->compare_join_context.false_transfer == nullptr) {
      continue;
    }

    const auto& join_context = compare_join_context->compare_join_context;
    if (&candidate_block != join_context.true_predecessor &&
        &candidate_block != join_context.false_predecessor) {
      continue;
    }

    const auto* join_transfer = join_context.join_transfer;
    if (c4c::backend::prepare::find_prepared_control_flow_block(
            control_flow, join_transfer->join_block_label) == nullptr) {
      return false;
    }
    if (c4c::backend::prepare::find_prepared_parallel_copy_bundle(
            control_flow,
            join_context.true_transfer->predecessor_label,
            join_context.true_transfer->successor_label) == nullptr ||
        c4c::backend::prepare::find_prepared_parallel_copy_bundle(
            control_flow,
            join_context.false_transfer->predecessor_label,
            join_context.false_transfer->successor_label) == nullptr) {
      return false;
    }
    return true;
  }

  return false;
}

bool prepared_compare_join_owns_branch_target_block(
    c4c::backend::prepare::PreparedNameTables& mutable_names,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& candidate_block,
    c4c::BlockLabelId candidate_label) {
  if (candidate_label == c4c::kInvalidBlockLabel) {
    return false;
  }

  if (find_bir_block_by_prepared_label(function, bir_names, mutable_names, candidate_label) !=
      &candidate_block) {
    return false;
  }

  for (const auto& branch_condition : control_flow.branch_conditions) {
    if (branch_condition.block_label == c4c::kInvalidBlockLabel ||
        !branch_condition.predicate.has_value() || !branch_condition.compare_type.has_value() ||
        !branch_condition.lhs.has_value() || !branch_condition.rhs.has_value() ||
        (*branch_condition.predicate != c4c::backend::bir::BinaryOpcode::Eq &&
         *branch_condition.predicate != c4c::backend::bir::BinaryOpcode::Ne) ||
        *branch_condition.compare_type != c4c::backend::bir::TypeKind::I32 ||
        (branch_condition.true_label != candidate_label &&
         branch_condition.false_label != candidate_label)) {
      continue;
    }
    const auto authoritative_join_transfer =
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            mutable_names, control_flow, branch_condition.block_label);
    if (!authoritative_join_transfer.has_value() ||
        authoritative_join_transfer->join_transfer == nullptr ||
        authoritative_join_transfer->true_transfer == nullptr ||
        authoritative_join_transfer->false_transfer == nullptr) {
      continue;
    }

    const auto* join_transfer = authoritative_join_transfer->join_transfer;
    if (c4c::backend::prepare::effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
            c4c::backend::prepare::PreparedJoinTransferCarrierKind::None ||
        authoritative_join_transfer->true_transfer->predecessor_label ==
            c4c::kInvalidBlockLabel ||
        authoritative_join_transfer->true_transfer->successor_label == c4c::kInvalidBlockLabel ||
        authoritative_join_transfer->false_transfer->predecessor_label ==
            c4c::kInvalidBlockLabel ||
        authoritative_join_transfer->false_transfer->successor_label == c4c::kInvalidBlockLabel) {
      continue;
    }
    if (c4c::backend::prepare::find_prepared_control_flow_block(
            control_flow, join_transfer->join_block_label) == nullptr) {
      return false;
    }
    return true;
  }

  return false;
}

void validate_prepared_control_flow_handoff(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function) {
  if (module.control_flow.functions.empty()) {
    return;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared function id");
  }

  const auto* control_flow = c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, *function_name);
  if (control_flow == nullptr) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared control-flow record");
  }

  auto mutable_names = module.names;
  for (const auto& block : function.blocks) {
    const auto block_label = bir_block_label_id(module.module.names, module.names, block);
    if (!block_label.has_value()) {
      throw_prepared_control_flow_handoff_error("BIR block '" + block.label +
                                                "' has no prepared block id");
    }
    if (c4c::backend::prepare::find_prepared_control_flow_block(*control_flow, *block_label) ==
        nullptr &&
        !prepared_compare_join_owns_branch_target_block(
            mutable_names, module.module.names, *control_flow, function, block, *block_label) &&
        !prepared_compare_join_owns_predecessor_block(
            mutable_names, module.module.names, *control_flow, function, block)) {
      throw_prepared_control_flow_handoff_error("BIR block '" + block.label +
                                                "' has no prepared control-flow block");
    }
  }

  for (const auto& block : control_flow->blocks) {
    validate_prepared_block_targets(
        function, module.module.names, mutable_names, *control_flow, block);
  }
  for (const auto& condition : control_flow->branch_conditions) {
    validate_prepared_branch_condition(
        function, module.module.names, mutable_names, *control_flow, condition);
  }
}

const c4c::backend::bir::Block* find_prepared_loop_countdown_block(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::BlockLabelId label) {
  return find_bir_block_by_prepared_label(function, bir_names, names, label);
}

const c4c::backend::bir::Block* require_prepared_loop_countdown_branch_target(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::NameTables& bir_names,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Block& source_block) {
  const auto source_label = bir_block_label_id(bir_names, names, source_block);
  if (!source_label.has_value()) {
    throw_prepared_control_flow_handoff_error(
        "loop-countdown branch source has no prepared block id");
  }

  const auto* prepared_block =
      c4c::backend::prepare::find_prepared_control_flow_block(control_flow, *source_label);
  if (prepared_block == nullptr ||
      prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
      prepared_block->branch_target_label == c4c::kInvalidBlockLabel) {
    throw_prepared_control_flow_handoff_error(
        "loop-countdown branch source has no authoritative prepared branch target");
  }

  const auto* target_block = find_prepared_loop_countdown_block(
      function, bir_names, names, prepared_block->branch_target_label);
  if (target_block == nullptr || target_block == &source_block) {
    throw_prepared_control_flow_handoff_error(
        "loop-countdown prepared branch target does not name a BIR block by id");
  }
  return target_block;
}

bool prepared_loop_countdown_condition_matches(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    const c4c::backend::bir::BinaryInst& compare,
    std::string_view counter_name) {
  if (!condition.predicate.has_value() || !condition.compare_type.has_value() ||
      !condition.lhs.has_value() || !condition.rhs.has_value() ||
      *condition.compare_type != c4c::backend::bir::TypeKind::I32 ||
      condition.condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
      condition.condition_value.name != compare.result.name) {
    return false;
  }

  const auto names_counter = [&](const c4c::backend::bir::Value& value) {
    return value.kind == c4c::backend::bir::Value::Kind::Named &&
           value.type == c4c::backend::bir::TypeKind::I32 &&
           value.name == counter_name;
  };
  const auto is_zero = [](const c4c::backend::bir::Value& value) {
    return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
           value.type == c4c::backend::bir::TypeKind::I32 && value.immediate == 0;
  };
  return (names_counter(*condition.lhs) && is_zero(*condition.rhs)) ||
         (names_counter(*condition.rhs) && is_zero(*condition.lhs));
}

bool append_prepared_loop_join_countdown_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (!function.params.empty() || function.blocks.empty() ||
      function.return_type != c4c::backend::bir::TypeKind::I32 ||
      c4c::backend::x86::abi::resolve_target_profile(module).arch != c4c::TargetArch::X86_64) {
    return false;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    return false;
  }
  const auto* control_flow = c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, *function_name);
  if (control_flow == nullptr) {
    return false;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
    return false;
  }

  bool saw_related_loop_contract = false;
  for (const auto& candidate_join_transfer : control_flow->join_transfers) {
    const auto* loop_block = find_prepared_loop_countdown_block(function,
                                                                module.module.names,
                                                                module.names,
                                                                candidate_join_transfer.join_block_label);
    if (loop_block == nullptr || loop_block == &entry ||
        loop_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        loop_block->insts.empty()) {
      continue;
    }

    const auto* loop_carrier =
        std::get_if<c4c::backend::bir::LoadLocalInst>(&loop_block->insts.front());
    const auto* loop_compare =
        std::get_if<c4c::backend::bir::BinaryInst>(&loop_block->insts.back());
    if (loop_carrier == nullptr || loop_compare == nullptr ||
        loop_carrier->result.kind != c4c::backend::bir::Value::Kind::Named ||
        loop_carrier->result.type != c4c::backend::bir::TypeKind::I32 ||
        loop_carrier->byte_offset != 0 || loop_carrier->address.has_value()) {
      continue;
    }

    const auto loop_label = bir_block_label_id(module.module.names, module.names, *loop_block);
    if (!loop_label.has_value()) {
      continue;
    }
    const auto* branch_condition =
        c4c::backend::prepare::find_prepared_branch_condition(*control_flow, *loop_label);
    if (branch_condition == nullptr) {
      continue;
    }
    saw_related_loop_contract = true;

    if (candidate_join_transfer.kind !=
            c4c::backend::prepare::PreparedJoinTransferKind::LoopCarry ||
        candidate_join_transfer.result.kind != c4c::backend::bir::Value::Kind::Named ||
        candidate_join_transfer.result.type != c4c::backend::bir::TypeKind::I32 ||
        candidate_join_transfer.result.name != loop_carrier->result.name ||
        candidate_join_transfer.edge_transfers.size() != 2) {
      throw_prepared_control_flow_handoff_error(
          "loop-countdown join transfer drifted from authoritative loop-carry metadata");
    }
    if (!prepared_loop_countdown_condition_matches(
            *branch_condition, *loop_compare, candidate_join_transfer.result.name)) {
      throw_prepared_control_flow_handoff_error(
          "loop-countdown branch condition drifted from authoritative prepared metadata");
    }

    const auto counter_id = c4c::backend::prepare::resolve_prepared_value_name_id(
        module.names, candidate_join_transfer.result.name);
    const auto* join_edges =
        counter_id.has_value()
            ? c4c::backend::prepare::incoming_transfers_for_join(
                  module.names,
                  *control_flow,
                  candidate_join_transfer.join_block_label,
                  *counter_id)
            : nullptr;
    if (join_edges == nullptr || join_edges->size() != 2) {
      throw_prepared_control_flow_handoff_error(
          "loop-countdown join transfer has no authoritative incoming edge set");
    }

    std::string body_label_text;
    std::string exit_label_text;
    if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne) {
      body_label_text =
          std::string(c4c::backend::prepare::prepared_block_label(module.names,
                                                                  branch_condition->true_label));
      exit_label_text =
          std::string(c4c::backend::prepare::prepared_block_label(module.names,
                                                                  branch_condition->false_label));
    } else if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq) {
      body_label_text =
          std::string(c4c::backend::prepare::prepared_block_label(module.names,
                                                                  branch_condition->false_label));
      exit_label_text =
          std::string(c4c::backend::prepare::prepared_block_label(module.names,
                                                                  branch_condition->true_label));
    } else {
      return false;
    }

    const auto* body_block = find_prepared_loop_countdown_block(
        function,
        module.module.names,
        module.names,
        *branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne
            ? branch_condition->true_label
            : branch_condition->false_label);
    const auto* exit_block = find_prepared_loop_countdown_block(
        function,
        module.module.names,
        module.names,
        *branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne
            ? branch_condition->false_label
            : branch_condition->true_label);
    if (body_block == nullptr || exit_block == nullptr || body_block == &entry ||
        exit_block == &entry || body_block == loop_block || exit_block == loop_block ||
        body_block == exit_block ||
        body_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        require_prepared_loop_countdown_branch_target(function,
                                                      module.module.names,
                                                      module.names,
                                                      *control_flow,
                                                      *body_block) != loop_block ||
        exit_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !exit_block->terminator.value.has_value() || body_block->insts.empty()) {
      return false;
    }

    const auto& exit_value = *exit_block->terminator.value;
    const bool exit_returns_counter =
        exit_value.kind == c4c::backend::bir::Value::Kind::Named &&
        exit_value.type == c4c::backend::bir::TypeKind::I32 &&
        exit_value.name == candidate_join_transfer.result.name;
    const bool exit_returns_zero =
        exit_value.kind == c4c::backend::bir::Value::Kind::Immediate &&
        exit_value.type == c4c::backend::bir::TypeKind::I32 && exit_value.immediate == 0;
    const auto* body_update =
        std::get_if<c4c::backend::bir::BinaryInst>(&body_block->insts.front());
    if ((!exit_returns_counter && !exit_returns_zero) || body_update == nullptr ||
        body_update->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
        body_update->operand_type != c4c::backend::bir::TypeKind::I32 ||
        body_update->result.type != c4c::backend::bir::TypeKind::I32 ||
        body_update->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
        body_update->lhs.name != candidate_join_transfer.result.name ||
        body_update->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
        body_update->rhs.type != c4c::backend::bir::TypeKind::I32 ||
        body_update->rhs.immediate != 1) {
      return false;
    }

    const c4c::backend::prepare::PreparedEdgeValueTransfer* init_incoming = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* body_incoming = nullptr;
    const auto body_block_label = bir_block_label_id(module.module.names, module.names, *body_block);
    if (!body_block_label.has_value()) {
      throw_prepared_control_flow_handoff_error(
          "loop-countdown body block has no prepared block id");
    }
    for (const auto& incoming : *join_edges) {
      if (incoming.predecessor_label == *body_block_label) {
        body_incoming = &incoming;
      } else {
        init_incoming = &incoming;
      }
    }
    if (init_incoming == nullptr || body_incoming == nullptr ||
        c4c::backend::prepare::find_prepared_parallel_copy_bundle(
            *control_flow,
            init_incoming->predecessor_label,
            init_incoming->successor_label) == nullptr ||
        c4c::backend::prepare::find_prepared_parallel_copy_bundle(
            *control_flow,
            body_incoming->predecessor_label,
            body_incoming->successor_label) == nullptr) {
      throw_prepared_control_flow_handoff_error(
          "loop-countdown edge has no authoritative prepared parallel-copy bundle");
    }

    const auto* init_block = find_prepared_loop_countdown_block(function,
                                                                module.module.names,
                                                                module.names,
                                                                init_incoming->predecessor_label);
    const auto block_has_supported_init_handoff_carrier =
        [&](const c4c::backend::bir::Block& candidate) {
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
        [&](const c4c::backend::bir::Block& target_block) {
          const c4c::backend::bir::Block* predecessor = nullptr;
          for (const auto& candidate : function.blocks) {
            if (candidate.terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
              continue;
            }
            if (require_prepared_loop_countdown_branch_target(function,
                                                             module.module.names,
                                                             module.names,
                                                             *control_flow,
                                                             candidate) != &target_block) {
              continue;
            }
            if (predecessor != nullptr) {
              return static_cast<const c4c::backend::bir::Block*>(nullptr);
            }
            predecessor = &candidate;
          }
          return predecessor;
        };

    const bool entry_reaches_loop_through_supported_handoff_prefix = [&]() {
      if (init_block == nullptr) {
        return false;
      }
      if (init_block == &entry) {
        return require_prepared_loop_countdown_branch_target(function,
                                                            module.module.names,
                                                            module.names,
                                                            *control_flow,
                                                            entry) == loop_block &&
               block_has_supported_init_handoff_carrier(entry);
      }
      if (init_block == loop_block || init_block == body_block || init_block == exit_block ||
          init_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
          require_prepared_loop_countdown_branch_target(function,
                                                       module.module.names,
                                                       module.names,
                                                       *control_flow,
                                                       *init_block) != loop_block ||
          !block_has_supported_init_handoff_carrier(*init_block)) {
        return false;
      }

      const auto* predecessor = find_unique_transparent_branch_predecessor(*init_block);
      std::size_t transparent_prefix_depth = 0;
      while (predecessor != nullptr && predecessor != &entry) {
        if (predecessor == loop_block || predecessor == body_block ||
            predecessor == exit_block || predecessor == init_block ||
            !predecessor->insts.empty()) {
          return false;
        }
        predecessor = find_unique_transparent_branch_predecessor(*predecessor);
        if (++transparent_prefix_depth > function.blocks.size()) {
          return false;
        }
      }
      return predecessor == &entry && predecessor->insts.empty() &&
             predecessor->terminator.kind == c4c::backend::bir::TerminatorKind::Branch;
    }();

    if (!entry_reaches_loop_through_supported_handoff_prefix ||
        init_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        init_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
        init_incoming->incoming_value.immediate < 0 ||
        body_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Named ||
        body_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
        body_incoming->incoming_value.name != body_update->result.name) {
      return false;
    }

    const auto symbol_name = data.render_asm_symbol_name(function.name);
    out.append_line(".globl " + symbol_name);
    out.append_line(".type " + symbol_name + ", @function");
    out.append_line(symbol_name + ":");
    out.append_line("    mov eax, " +
                    std::to_string(static_cast<std::int32_t>(
                        init_incoming->incoming_value.immediate)));
    out.append_line(render_function_local_label(function.name, loop_block->label) + ":");
    out.append_line("    test eax, eax");
    out.append_line("    je " + render_function_local_label(function.name, exit_label_text));
    out.append_line(render_function_local_label(function.name, body_label_text) + ":");
    out.append_line("    sub eax, 1");
    out.append_line("    jmp " + render_function_local_label(function.name, loop_block->label));
    out.append_line(render_function_local_label(function.name, exit_label_text) + ":");
    if (exit_returns_zero) {
      out.append_line("    mov eax, 0");
    }
    out.append_line("    ret");
    return true;
  }

  if (saw_related_loop_contract) {
    throw_prepared_control_flow_handoff_error(
        "loop-countdown prepared control-flow contract is outside the supported x86 shape");
  }
  return false;
}

bool is_grouped_span(std::size_t contiguous_width,
                     const std::vector<std::string>& occupied_register_names) {
  return contiguous_width > 1 || occupied_register_names.size() > 1;
}

std::string render_grouped_span(c4c::backend::prepare::PreparedRegisterBank bank,
                                std::optional<std::string_view> register_name,
                                std::size_t contiguous_width,
                                const std::vector<std::string>& occupied_register_names) {
  std::string rendered =
      std::string(c4c::backend::prepare::prepared_register_bank_name(bank)) + ":";
  rendered += register_name.has_value() ? std::string(*register_name) : std::string("<none>");
  rendered += "/w" + std::to_string(contiguous_width);
  if (!occupied_register_names.empty()) {
    rendered += "[";
    for (std::size_t index = 0; index < occupied_register_names.size(); ++index) {
      if (index != 0) {
        rendered += ",";
      }
      rendered += occupied_register_names[index];
    }
    rendered += "]";
  }
  return rendered;
}

std::string render_optional_grouped_span(
    std::optional<c4c::backend::prepare::PreparedRegisterBank> bank,
    std::optional<std::string_view> register_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_register_names) {
  return render_grouped_span(bank.value_or(c4c::backend::prepare::PreparedRegisterBank::None),
                             register_name,
                             contiguous_width,
                             occupied_register_names);
}

void append_grouped_authority_comments(c4c::backend::x86::core::Text& out,
                                       const c4c::backend::prepare::PreparedBirModule& module,
                                       c4c::FunctionNameId function_name) {
  const auto consumed = c4c::backend::x86::consume_plans(module, function_name);
  const auto* frame_plan = consumed.frame;
  const auto* call_plans = consumed.calls;
  const auto* regalloc = consumed.regalloc;
  const auto* storage_plan = consumed.storage;

  std::size_t grouped_saved = 0;
  std::size_t grouped_preserved = 0;
  std::size_t grouped_clobbered = 0;
  std::size_t grouped_call_argument_spans = 0;
  std::size_t grouped_call_result_spans = 0;
  std::size_t grouped_spills = 0;
  std::size_t grouped_reloads = 0;
  std::size_t grouped_storage = 0;

  if (frame_plan != nullptr) {
    grouped_saved = static_cast<std::size_t>(std::count_if(
        frame_plan->saved_callee_registers.begin(),
        frame_plan->saved_callee_registers.end(),
        [](const c4c::backend::prepare::PreparedSavedRegister& saved) {
          return is_grouped_span(saved.contiguous_width, saved.occupied_register_names);
        }));
  }

  if (call_plans != nullptr) {
    for (const auto& call : call_plans->calls) {
      grouped_call_argument_spans += static_cast<std::size_t>(std::count_if(
          call.arguments.begin(),
          call.arguments.end(),
          [](const c4c::backend::prepare::PreparedCallArgumentPlan& argument) {
            return is_grouped_span(argument.destination_contiguous_width,
                                   argument.destination_occupied_register_names);
          }));
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        ++grouped_call_result_spans;
      }
      grouped_preserved += static_cast<std::size_t>(std::count_if(
          call.preserved_values.begin(),
          call.preserved_values.end(),
          [](const c4c::backend::prepare::PreparedCallPreservedValue& preserved) {
            return is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names);
          }));
      grouped_clobbered += static_cast<std::size_t>(std::count_if(
          call.clobbered_registers.begin(),
          call.clobbered_registers.end(),
          [](const c4c::backend::prepare::PreparedClobberedRegister& clobber) {
            return is_grouped_span(clobber.contiguous_width, clobber.occupied_register_names);
          }));
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      switch (op.op_kind) {
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Spill:
          ++grouped_spills;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Reload:
          ++grouped_reloads;
          break;
        case c4c::backend::prepare::PreparedSpillReloadOpKind::Rematerialize:
          break;
      }
    }
  }

  if (storage_plan != nullptr) {
    grouped_storage = static_cast<std::size_t>(std::count_if(
        storage_plan->values.begin(),
        storage_plan->values.end(),
        [](const c4c::backend::prepare::PreparedStoragePlanValue& value) {
          return is_grouped_span(value.contiguous_width, value.occupied_register_names);
        }));
  }

  if (grouped_saved == 0 && grouped_preserved == 0 && grouped_clobbered == 0 &&
      grouped_call_argument_spans == 0 && grouped_call_result_spans == 0 &&
      grouped_spills == 0 && grouped_reloads == 0 && grouped_storage == 0) {
    return;
  }

  out.append_line("    # grouped authority: saved=" + std::to_string(grouped_saved) +
                  " preserved=" + std::to_string(grouped_preserved) +
                  " clobbered=" + std::to_string(grouped_clobbered) +
                  " call_args=" + std::to_string(grouped_call_argument_spans) +
                  " call_results=" + std::to_string(grouped_call_result_spans) +
                  " spills=" + std::to_string(grouped_spills) +
                  " reloads=" + std::to_string(grouped_reloads) +
                  " storage=" + std::to_string(grouped_storage));

  if (frame_plan != nullptr) {
    for (const auto& saved : frame_plan->saved_callee_registers) {
      if (!is_grouped_span(saved.contiguous_width, saved.occupied_register_names)) {
        continue;
      }
      out.append_line("    # grouped saved save_index=" + std::to_string(saved.save_index) +
                      " span=" +
                      render_grouped_span(saved.bank,
                                          std::string_view(saved.register_name),
                                          saved.contiguous_width,
                                          saved.occupied_register_names));
    }
  }

  if (call_plans != nullptr) {
    for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
      const auto& call = call_plans->calls[call_index];
      for (const auto& argument : call.arguments) {
        if (!is_grouped_span(argument.destination_contiguous_width,
                             argument.destination_occupied_register_names)) {
          continue;
        }
        std::string line = "    # grouped arg call#" + std::to_string(call_index) +
                           " arg#" + std::to_string(argument.arg_index);
        if (argument.source_value_id.has_value()) {
          line += " source_value_id=" + std::to_string(*argument.source_value_id);
        }
        line += " dest_span=" +
                render_optional_grouped_span(argument.destination_register_bank,
                                             argument.destination_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *argument.destination_register_name}
                                                 : std::nullopt,
                                             argument.destination_contiguous_width,
                                             argument.destination_occupied_register_names);
        out.append_line(line);
      }
      if (call.result.has_value() &&
          is_grouped_span(call.result->source_contiguous_width,
                          call.result->source_occupied_register_names)) {
        const auto& result = *call.result;
        std::string line = "    # grouped result call#" + std::to_string(call_index);
        if (result.destination_value_id.has_value()) {
          line += " destination_value_id=" + std::to_string(*result.destination_value_id);
        }
        line += " source_span=" +
                render_optional_grouped_span(result.source_register_bank,
                                             result.source_register_name.has_value()
                                                 ? std::optional<std::string_view>{
                                                       *result.source_register_name}
                                                 : std::nullopt,
                                             result.source_contiguous_width,
                                             result.source_occupied_register_names);
        out.append_line(line);
      }
      for (const auto& preserved : call.preserved_values) {
        if (!is_grouped_span(preserved.contiguous_width, preserved.occupied_register_names)) {
          continue;
        }
        std::string line =
            "    # grouped preserve call#" + std::to_string(call_index) + " value=" +
            std::string(
                c4c::backend::prepare::prepared_value_name(module.names, preserved.value_name)) +
            " route=" +
            std::string(
                c4c::backend::prepare::prepared_call_preservation_route_name(preserved.route)) +
            " span=" +
            render_grouped_span(
                preserved.register_bank.value_or(
                    c4c::backend::prepare::PreparedRegisterBank::None),
                preserved.register_name.has_value()
                    ? std::optional<std::string_view>{*preserved.register_name}
                    : std::nullopt,
                preserved.contiguous_width,
                preserved.occupied_register_names);
        if (preserved.callee_saved_save_index.has_value()) {
          line += " save_index=" + std::to_string(*preserved.callee_saved_save_index);
        }
        out.append_line(line);
      }
      for (const auto& clobbered : call.clobbered_registers) {
        if (!is_grouped_span(clobbered.contiguous_width, clobbered.occupied_register_names)) {
          continue;
        }
        out.append_line("    # grouped clobber call#" + std::to_string(call_index) +
                        " span=" +
                        render_grouped_span(clobbered.bank,
                                            std::string_view(clobbered.register_name),
                                            clobbered.contiguous_width,
                                            clobbered.occupied_register_names));
      }
    }
  }

  if (regalloc != nullptr) {
    for (const auto& op : regalloc->spill_reload_ops) {
      if (!is_grouped_span(op.contiguous_width, op.occupied_register_names)) {
        continue;
      }
      if (op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Spill &&
          op.op_kind != c4c::backend::prepare::PreparedSpillReloadOpKind::Reload) {
        continue;
      }
      std::string line =
          "    # grouped " +
          std::string(c4c::backend::prepare::prepared_spill_reload_op_kind_name(op.op_kind)) +
          " value_id=" + std::to_string(op.value_id);
      if (const auto value_it = std::find_if(
              regalloc->values.begin(),
              regalloc->values.end(),
              [&op](const c4c::backend::prepare::PreparedRegallocValue& value) {
                return value.value_id == op.value_id;
              });
          value_it != regalloc->values.end()) {
        line += " value=" +
                std::string(
                    c4c::backend::prepare::prepared_value_name(module.names, value_it->value_name));
      }
      line += " span=" +
              render_grouped_span(op.register_bank,
                                  op.register_name.has_value()
                                      ? std::optional<std::string_view>{*op.register_name}
                                      : std::nullopt,
                                  op.contiguous_width,
                                  op.occupied_register_names);
      if (op.slot_id.has_value()) {
        line += " slot_id=#" + std::to_string(*op.slot_id);
      }
      if (op.stack_offset_bytes.has_value()) {
        line += " stack_offset=" + std::to_string(*op.stack_offset_bytes);
      }
      out.append_line(line);
    }
  }

  if (storage_plan != nullptr) {
    for (const auto& value : storage_plan->values) {
      if (!is_grouped_span(value.contiguous_width, value.occupied_register_names)) {
        continue;
      }
      out.append_line(
          "    # grouped storage value=" +
          std::string(c4c::backend::prepare::prepared_value_name(module.names, value.value_name)) +
          " span=" +
          render_grouped_span(value.bank,
                              value.register_name.has_value()
                                  ? std::optional<std::string_view>{*value.register_name}
                                  : std::nullopt,
                              value.contiguous_width,
                              value.occupied_register_names));
    }
  }
}

bool block_defines_named_value(const c4c::backend::bir::Block& block, std::string_view name) {
  if (name.empty()) {
    return false;
  }
  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary != nullptr && binary->result.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->result.name == name) {
      return true;
    }
  }
  return false;
}

const c4c::backend::bir::Global* find_supported_same_module_i32_global(
    const c4c::backend::bir::Module& module,
    std::string_view global_name,
    std::size_t byte_offset) {
  for (const auto& global : module.globals) {
    if (global.name != global_name || global.is_extern || global.is_thread_local) {
      continue;
    }
    if (byte_offset > global.size_bytes || global.size_bytes - byte_offset < 4) {
      return nullptr;
    }
    return &global;
  }
  return nullptr;
}

std::optional<std::int32_t> fold_supported_pointer_compare_return(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function) {
  if (function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::int32_t> folded_i32_values;
  std::unordered_map<std::string_view, std::size_t> symbolic_pointer_roots;
  std::size_t next_symbolic_pointer_root = 1;

  const auto intern_symbolic_pointer_root = [&](std::string_view value_name) -> std::size_t {
    const auto [it, inserted] =
        symbolic_pointer_roots.emplace(value_name, next_symbolic_pointer_root);
    if (inserted) {
      ++next_symbolic_pointer_root;
    }
    return it->second;
  };

  const auto resolve_pointer = [&](const c4c::backend::bir::Value& value)
      -> std::optional<std::size_t> {
    if (value.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      if (value.immediate < 0) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(value.immediate);
    }
    const auto is_pointer_param = std::any_of(
        function.params.begin(), function.params.end(), [&](const c4c::backend::bir::Param& param) {
          return param.type == c4c::backend::bir::TypeKind::Ptr && param.name == value.name;
        });
    if (is_pointer_param || block_defines_named_value(block, value.name)) {
      return std::nullopt;
    }
    if (!value.name.empty() && value.name.front() == '@') {
      const std::string_view symbol_name(value.name.data() + 1, value.name.size() - 1);
      const auto names_string_constant =
          std::any_of(module.string_constants.begin(),
                      module.string_constants.end(),
                      [&](const c4c::backend::bir::StringConstant& constant) {
                        return constant.name == symbol_name;
                      });
      const auto names_global =
          std::any_of(module.globals.begin(),
                      module.globals.end(),
                      [&](const c4c::backend::bir::Global& global) {
                        return global.name == symbol_name;
                      });
      if (!names_string_constant && !names_global) {
        return std::nullopt;
      }
    }
    return intern_symbolic_pointer_root(value.name);
  };

  for (const auto& inst : block.insts) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        binary->operand_type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto lhs = resolve_pointer(binary->lhs);
    const auto rhs = resolve_pointer(binary->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    switch (binary->opcode) {
      case c4c::backend::bir::BinaryOpcode::Eq:
        folded_i32_values.emplace(binary->result.name, *lhs == *rhs ? 1 : 0);
        break;
      case c4c::backend::bir::BinaryOpcode::Ne:
        folded_i32_values.emplace(binary->result.name, *lhs != *rhs ? 1 : 0);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto& return_value = *block.terminator.value;
  if (return_value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto found = folded_i32_values.find(return_value.name);
  if (found == folded_i32_values.end()) {
    return std::nullopt;
  }
  return found->second;
}

const c4c::backend::prepare::PreparedValueLocationFunction*
require_prepared_value_location_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function) {
  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared function id");
  }
  for (const auto& function_locations : module.value_locations.functions) {
    if (function_locations.function_name == *function_name) {
      return &function_locations;
    }
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has no prepared value-location record");
}

const c4c::backend::prepare::PreparedMoveResolution& require_prepared_i32_return_move(
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    std::size_t block_index) {
  const auto* before_return = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations,
      c4c::backend::prepare::PreparedMovePhase::BeforeReturn,
      block_index,
      block.insts.size());
  if (before_return == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return move bundle");
  }
  if (before_return->moves.size() != 1) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an ambiguous prepared return move bundle");
  }
  const auto& move = before_return->moves.front();
  if (move.destination_kind !=
          c4c::backend::prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::Register ||
      !move.destination_register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared return ABI move");
  }
  return move;
}

const c4c::backend::prepare::PreparedMoveResolution& require_prepared_i32_instruction_move(
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::size_t block_index,
    std::size_t instruction_index) {
  const auto* before_instruction = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations,
      c4c::backend::prepare::PreparedMovePhase::BeforeInstruction,
      block_index,
      instruction_index);
  if (before_instruction == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared instruction move bundle");
  }
  if (before_instruction->moves.size() != 1) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an ambiguous prepared instruction move bundle");
  }
  const auto& move = before_instruction->moves.front();
  if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::Value) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared instruction move");
  }
  return move;
}

const c4c::backend::prepare::PreparedValueHome& require_prepared_i32_value_home(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::string_view value_name,
    std::string_view context) {
  const auto prepared_value_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names, value_name);
  if (!prepared_value_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name + "' " +
                                                std::string(context) +
                                                " has no prepared name id");
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(function_locations, *prepared_value_name);
  if (home == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name + "' " +
                                                std::string(context) + " has no prepared home");
  }
  return *home;
}

std::string require_prepared_i32_register_home(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::string_view value_name,
    std::string_view context) {
  const auto& home =
      require_prepared_i32_value_home(module, function_locations, function, value_name, context);
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared " +
                                                std::string(context) + " home");
  }
  return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
}

std::string render_prepared_i32_value_home_operand(
    const c4c::backend::prepare::PreparedValueHome& home,
    const c4c::backend::bir::Function& function) {
  switch (home.kind) {
    case c4c::backend::prepare::PreparedValueHomeKind::Register:
      if (!home.register_name.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a register home without a register");
      }
      return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
    case c4c::backend::prepare::PreparedValueHomeKind::StackSlot:
      if (!home.offset_bytes.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a stack home without an offset");
      }
      return render_stack_memory_operand(*home.offset_bytes, "DWORD");
    case c4c::backend::prepare::PreparedValueHomeKind::None:
    case c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate:
    case c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset:
      break;
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has an unsupported prepared i32 return home");
}

bool append_prepared_i32_immediate_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (!block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  if (!function.return_abi.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return ABI");
  }
  const auto return_register = c4c::backend::prepare::call_result_destination_register_name(
      c4c::backend::x86::abi::resolve_target_profile(module), *function.return_abi);
  if (!return_register.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared register return ABI destination");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(*return_register) + ", " +
                  std::to_string(block.terminator.value->immediate));
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_passthrough_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (!block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto value_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names,
                                                            block.terminator.value->name);
  if (!value_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' returns a value with no prepared name id");
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(*function_locations, *value_name);
  if (home == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' returns a value with no prepared home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != home->value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from value home");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    sub rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + render_prepared_i32_value_home_operand(*home, function));
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    add rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    ret");
  return true;
}

std::optional<std::string_view> supported_i32_binary_immediate_mnemonic(
    c4c::backend::bir::BinaryOpcode opcode);

bool append_prepared_i32_rematerialized_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() != 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.front());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.name != block.terminator.value->name ||
      binary->lhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      binary->lhs.type != c4c::backend::bir::TypeKind::I32 ||
      binary->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      binary->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      !supported_i32_binary_immediate_mnemonic(binary->opcode).has_value()) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& home = require_prepared_i32_value_home(module,
                                                     *function_locations,
                                                     function,
                                                     block.terminator.value->name,
                                                     "return value");
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate) {
    return false;
  }
  if (!home.immediate_i32.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has a rematerializable return without an immediate");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from rematerialized home");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + std::to_string(*home.immediate_i32));
  out.append_line("    ret");
  return true;
}

std::optional<std::string_view> supported_i32_binary_immediate_mnemonic(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return "add";
    case c4c::backend::bir::BinaryOpcode::Sub:
      return "sub";
    case c4c::backend::bir::BinaryOpcode::Mul:
      return "imul";
    case c4c::backend::bir::BinaryOpcode::And:
      return "and";
    case c4c::backend::bir::BinaryOpcode::Or:
      return "or";
    case c4c::backend::bir::BinaryOpcode::Xor:
      return "xor";
    case c4c::backend::bir::BinaryOpcode::Shl:
      return "shl";
    case c4c::backend::bir::BinaryOpcode::LShr:
      return "shr";
    case c4c::backend::bir::BinaryOpcode::AShr:
      return "sar";
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      break;
  }
  return std::nullopt;
}

bool is_commutative_i32_binary_immediate(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
      return true;
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

bool append_prepared_i32_binary_immediate_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() != 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.front());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      binary->result.name != block.terminator.value->name) {
    return false;
  }

  const auto mnemonic = supported_i32_binary_immediate_mnemonic(binary->opcode);
  if (!mnemonic.has_value()) {
    return false;
  }

  const c4c::backend::bir::Value* named_operand = nullptr;
  const c4c::backend::bir::Value* immediate_operand = nullptr;
  if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
    named_operand = &binary->lhs;
    immediate_operand = &binary->rhs;
  } else if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
             binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
             is_commutative_i32_binary_immediate(binary->opcode)) {
    named_operand = &binary->rhs;
    immediate_operand = &binary->lhs;
  } else {
    return false;
  }

  if (named_operand->type != c4c::backend::bir::TypeKind::I32 ||
      immediate_operand->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& source_home = require_prepared_i32_value_home(
      module, *function_locations, function, named_operand->name, "binary source value");
  const auto& result_home = require_prepared_i32_value_home(
      module, *function_locations, function, binary->result.name, "binary result value");

  const auto& instruction_move =
      require_prepared_i32_instruction_move(*function_locations, function, 0, 0);
  if (instruction_move.from_value_id != source_home.value_id ||
      instruction_move.to_value_id != result_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' instruction move drifted from value homes");
  }
  if (instruction_move.destination_register_name.has_value() &&
      result_home.register_name.has_value() &&
      *instruction_move.destination_register_name != *result_home.register_name) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' instruction move destination drifted from result home");
  }
  if (result_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !result_home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared i32 binary result home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != result_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from result home");
  }

  const auto result_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*result_home.register_name);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    sub rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    mov " + result_register + ", " +
                  render_prepared_i32_value_home_operand(source_home, function));
  out.append_line("    " + std::string(*mnemonic) + " " + result_register + ", " +
                  std::to_string(immediate_operand->immediate));
  out.append_line("    mov " +
                  c4c::backend::x86::abi::narrow_i32_register_name(
                      *return_move.destination_register_name) +
                  ", " + result_register);
  if (module.stack_layout.frame_size_bytes != 0) {
    out.append_line("    add rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_same_module_global_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.size() < 1 ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* load_global =
      std::get_if<c4c::backend::bir::LoadGlobalInst>(&block.insts.back());
  if (load_global == nullptr ||
      load_global->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load_global->result.type != c4c::backend::bir::TypeKind::I32 ||
      load_global->result.name != block.terminator.value->name ||
      load_global->address.has_value() ||
      find_supported_same_module_i32_global(
          module.module, load_global->global_name, load_global->byte_offset) == nullptr) {
    return false;
  }

  for (std::size_t inst_index = 0; inst_index + 1 < block.insts.size(); ++inst_index) {
    const auto* store_global =
        std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index]);
    if (store_global == nullptr ||
        store_global->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        store_global->value.type != c4c::backend::bir::TypeKind::I32 ||
        store_global->address.has_value() ||
        find_supported_same_module_i32_global(
            module.module, store_global->global_name, store_global->byte_offset) == nullptr) {
      return false;
    }
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto& load_home = require_prepared_i32_value_home(module,
                                                         *function_locations,
                                                         function,
                                                         load_global->result.name,
                                                         "global load result value");
  if (load_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !load_home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared i32 global load home");
  }

  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != load_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from global load home");
  }

  const auto return_register = c4c::backend::x86::abi::narrow_i32_register_name(
      *return_move.destination_register_name);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  for (std::size_t inst_index = 0; inst_index + 1 < block.insts.size(); ++inst_index) {
    const auto& store_global =
        *std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index]);
    out.append_line("    mov " +
                    render_global_i32_memory_operand(
                        data, store_global.global_name, store_global.byte_offset) +
                    ", " + std::to_string(store_global.value.immediate));
  }
  out.append_line("    mov " + return_register + ", " +
                  render_global_i32_memory_operand(
                      data, load_global->global_name, load_global->byte_offset));
  out.append_line("    ret");
  return true;
}

bool append_prepared_i32_same_module_global_sub_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  std::unordered_map<std::string_view, std::string> value_locations;
  std::unordered_map<std::string_view, const c4c::backend::prepare::PreparedValueHome*> value_homes;
  std::string current_eax_value;

  const auto require_register_home =
      [&](std::string_view value_name,
          std::string_view context) -> const c4c::backend::prepare::PreparedValueHome& {
    const auto& home =
        require_prepared_i32_value_home(module, *function_locations, function, value_name, context);
    if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
        !home.register_name.has_value()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has an unsupported prepared i32 " +
                                                  std::string(context) + " home");
    }
    value_homes[value_name] = &home;
    return home;
  };

  const auto narrow_home_register =
      [](const c4c::backend::prepare::PreparedValueHome& home) -> std::string {
    return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
  };

  const auto value_location = [&](std::string_view value_name) -> std::optional<std::string> {
    const auto found = value_locations.find(value_name);
    if (found == value_locations.end()) {
      return std::nullopt;
    }
    return found->second;
  };

  const auto value_is_used_later = [&](std::string_view value_name,
                                       std::size_t after_inst_index) -> bool {
    for (std::size_t later_index = after_inst_index + 1; later_index < block.insts.size();
         ++later_index) {
      const auto* later_binary =
          std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[later_index]);
      if (later_binary == nullptr) {
        continue;
      }
      if ((later_binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
           later_binary->lhs.name == value_name) ||
          (later_binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
           later_binary->rhs.name == value_name)) {
        return true;
      }
    }
    return false;
  };

  const auto validate_binary_operand_move =
      [&](const c4c::backend::bir::Value& operand,
          const c4c::backend::prepare::PreparedValueHome& result_home,
          const c4c::backend::prepare::PreparedMoveBundle& bundle) {
    if (operand.kind != c4c::backend::bir::Value::Kind::Named) {
      return;
    }
    const auto home_found = value_homes.find(operand.name);
    if (home_found == value_homes.end()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' binary operand has no tracked prepared home");
    }
    const auto matching_move =
        std::find_if(bundle.moves.begin(),
                     bundle.moves.end(),
                     [&](const c4c::backend::prepare::PreparedMoveResolution& move) {
                       return move.from_value_id == home_found->second->value_id &&
                              move.to_value_id == result_home.value_id &&
                              move.destination_kind ==
                                  c4c::backend::prepare::PreparedMoveDestinationKind::Value &&
                              move.destination_storage_kind ==
                                  c4c::backend::prepare::PreparedMoveStorageKind::Register;
                     });
    if (matching_move == bundle.moves.end()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' instruction move drifted from binary operand homes");
    }
  };

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");

  bool saw_binary_sub = false;
  for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
    if (const auto* store_global =
            std::get_if<c4c::backend::bir::StoreGlobalInst>(&block.insts[inst_index])) {
      if (store_global->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store_global->value.type != c4c::backend::bir::TypeKind::I32 ||
          store_global->address.has_value() ||
          find_supported_same_module_i32_global(
              module.module, store_global->global_name, store_global->byte_offset) == nullptr) {
        return false;
      }
      function_out.append_line("    mov " +
                               render_global_i32_memory_operand(
                                   data, store_global->global_name, store_global->byte_offset) +
                               ", " + std::to_string(store_global->value.immediate));
      continue;
    }

    if (const auto* load_global =
            std::get_if<c4c::backend::bir::LoadGlobalInst>(&block.insts[inst_index])) {
      if (load_global->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load_global->result.type != c4c::backend::bir::TypeKind::I32 ||
          load_global->address.has_value() ||
          find_supported_same_module_i32_global(
              module.module, load_global->global_name, load_global->byte_offset) == nullptr) {
        return false;
      }
      require_register_home(load_global->result.name, "global load result value");
      function_out.append_line("    mov eax, " +
                               render_global_i32_memory_operand(
                                   data, load_global->global_name, load_global->byte_offset));
      value_locations[load_global->result.name] = "eax";
      current_eax_value = load_global->result.name;
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[inst_index]);
    if (binary == nullptr || binary->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
        binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        binary->lhs.type != c4c::backend::bir::TypeKind::I32 ||
        binary->rhs.type != c4c::backend::bir::TypeKind::I32) {
      return false;
    }

    const auto& result_home = require_register_home(binary->result.name, "binary result value");
    const auto* instruction_move = c4c::backend::prepare::find_prepared_move_bundle(
        *function_locations,
        c4c::backend::prepare::PreparedMovePhase::BeforeInstruction,
        0,
        inst_index);
    if (instruction_move == nullptr) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has no prepared binary instruction move bundle");
    }
    const std::size_t named_operand_count =
        (binary->lhs.kind == c4c::backend::bir::Value::Kind::Named ? 1U : 0U) +
        (binary->rhs.kind == c4c::backend::bir::Value::Kind::Named ? 1U : 0U);
    if (instruction_move->moves.size() != named_operand_count) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has an ambiguous prepared binary instruction move bundle");
    }
    validate_binary_operand_move(binary->lhs, result_home, *instruction_move);
    validate_binary_operand_move(binary->rhs, result_home, *instruction_move);

    std::string rhs_operand;
    if (binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
      rhs_operand = std::to_string(binary->rhs.immediate);
    } else {
      const auto rhs_location = value_location(binary->rhs.name);
      if (!rhs_location.has_value()) {
        return false;
      }
      function_out.append_line("    mov ecx, " + *rhs_location);
      rhs_operand = "ecx";
    }

    if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate) {
      function_out.append_line("    mov eax, " + std::to_string(binary->lhs.immediate));
    } else {
      const auto lhs_location = value_location(binary->lhs.name);
      if (!lhs_location.has_value()) {
        return false;
      }
      if (*lhs_location != "eax") {
        function_out.append_line("    mov eax, " + *lhs_location);
      }
    }
    function_out.append_line("    sub eax, " + rhs_operand);

    const auto result_register = narrow_home_register(result_home);
    if (result_register != "eax") {
      function_out.append_line("    mov " + result_register + ", eax");
    }
    if (value_is_used_later(binary->result.name, inst_index)) {
      function_out.append_line("    mov ecx, eax");
    }
    value_locations[binary->result.name] = result_register;
    current_eax_value = binary->result.name;
    saw_binary_sub = true;
  }

  if (!saw_binary_sub) {
    return false;
  }

  const auto return_location = value_location(block.terminator.value->name);
  if (!return_location.has_value()) {
    return false;
  }
  const auto return_home_found = value_homes.find(block.terminator.value->name);
  if (return_home_found == value_homes.end()) {
    return false;
  }
  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != return_home_found->second->value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' return move source drifted from expression home");
  }
  const auto return_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*return_move.destination_register_name);
  if (current_eax_value != block.terminator.value->name && *return_location != return_register) {
    function_out.append_line("    mov " + return_register + ", " + *return_location);
  }
  function_out.append_line("    ret");
  out.append_raw(function_out.take_text());
  return true;
}

std::optional<std::string> render_prepared_i32_zero_test_source(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Param& param,
    c4c::backend::x86::core::Text& function_out) {
  const auto& home = require_prepared_i32_value_home(
      module, function_locations, function, param.name, "compare branch entry value");
  switch (home.kind) {
    case c4c::backend::prepare::PreparedValueHomeKind::Register:
      if (!home.register_name.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a register entry home without a register");
      }
      return c4c::backend::x86::abi::narrow_i32_register_name(*home.register_name);
    case c4c::backend::prepare::PreparedValueHomeKind::StackSlot:
      if (!home.offset_bytes.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has a stack entry home without an offset");
      }
      if (module.stack_layout.frame_size_bytes != 0) {
        function_out.append_line("    sub rsp, " +
                                 std::to_string(module.stack_layout.frame_size_bytes));
      }
      function_out.append_line("    mov eax, " +
                               render_stack_memory_operand(*home.offset_bytes, "DWORD"));
      function_out.append_line("    test eax, eax");
      if (module.stack_layout.frame_size_bytes != 0) {
        function_out.append_line("    add rsp, " +
                                 std::to_string(module.stack_layout.frame_size_bytes));
      }
      return std::nullopt;
    case c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate:
      if (!home.immediate_i32.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' has a rematerializable compare branch entry without an immediate");
      }
      function_out.append_line("    mov eax, " + std::to_string(*home.immediate_i32));
      function_out.append_line("    test eax, eax");
      return std::nullopt;
    case c4c::backend::prepare::PreparedValueHomeKind::None:
    case c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset:
      break;
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has an unsupported prepared compare branch entry home");
}

void append_prepared_i32_leaf_return(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    std::size_t block_index,
    const c4c::backend::bir::Value& return_value) {
  if (!function.return_abi.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return ABI");
  }
  const auto return_register = c4c::backend::prepare::call_result_destination_register_name(
      c4c::backend::x86::abi::resolve_target_profile(module), *function.return_abi);
  if (!return_register.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared register return ABI destination");
  }
  const auto narrow_return_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*return_register);

  if (return_value.kind == c4c::backend::bir::Value::Kind::Immediate &&
      return_value.type == c4c::backend::bir::TypeKind::I32) {
    function_out.append_line("    mov " + narrow_return_register + ", " +
                             std::to_string(return_value.immediate));
    function_out.append_line("    ret");
    return;
  }

  if (return_value.kind != c4c::backend::bir::Value::Kind::Named ||
      return_value.type != c4c::backend::bir::TypeKind::I32) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported compare branch leaf return");
  }

  const auto& return_home = require_prepared_i32_value_home(
      module, function_locations, function, return_value.name, "compare branch leaf return value");
  const auto& return_move =
      require_prepared_i32_return_move(function_locations, function, block, block_index);
  if (return_move.from_value_id != return_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' compare branch leaf return move source drifted from value home");
  }
  function_out.append_line("    mov " +
                           c4c::backend::x86::abi::narrow_i32_register_name(
                               *return_move.destination_register_name) +
                           ", " + render_prepared_i32_value_home_operand(return_home, function));
  function_out.append_line("    ret");
}

void require_prepared_compare_join_parallel_copy(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label) {
  const auto* parallel_copy =
      c4c::backend::prepare::find_prepared_parallel_copy_bundle(
          control_flow, predecessor_label, successor_label);
  if (parallel_copy == nullptr) {
    throw_prepared_control_flow_handoff_error(
        "compare-join edge has no authoritative prepared parallel-copy bundle");
  }
  const auto* move_bundle =
      c4c::backend::prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
          names, function, function_locations, *parallel_copy);
  if (move_bundle == nullptr ||
      !c4c::backend::prepare::prepared_move_bundle_has_out_of_ssa_parallel_copy_authority(
          *move_bundle)) {
    throw_prepared_value_location_handoff_error(
        "compare-join edge has no authoritative prepared out-of-SSA move bundle");
  }
  for (std::size_t step_index = 0; step_index < parallel_copy->steps.size(); ++step_index) {
    const auto* parallel_copy_move =
        c4c::backend::prepare::find_prepared_parallel_copy_move_for_step(
            *parallel_copy, step_index);
    const auto* value_location_move =
        c4c::backend::prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(
            *move_bundle, step_index);
    if (parallel_copy_move == nullptr || value_location_move == nullptr) {
      throw_prepared_value_location_handoff_error(
          "compare-join edge parallel-copy step drifted from prepared move authority");
    }
    if (parallel_copy_move->source_value.kind ==
            c4c::backend::bir::Value::Kind::Immediate &&
        parallel_copy_move->source_value.type == c4c::backend::bir::TypeKind::I32 &&
        value_location_move->source_immediate_i32 != parallel_copy_move->source_value.immediate) {
      throw_prepared_value_location_handoff_error(
          "compare-join edge immediate source drifted from prepared move authority");
    }
  }
}

void append_prepared_i32_compare_join_return_arm(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const Data& data,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& join_block,
    std::size_t join_block_index,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm& return_arm) {
  const auto& return_context = return_arm.arm.context;
  const auto& return_move =
      require_prepared_i32_return_move(function_locations, function, join_block, join_block_index);
  const auto return_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*return_move.destination_register_name);
  if (return_context.block_index != join_block_index ||
      return_context.instruction_index > join_block.insts.size()) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' compare-join return context drifted from prepared join block");
  }
  if (join_block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !join_block.terminator.value.has_value() ||
      join_block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      join_block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' has an unsupported compare-join return terminator");
  }
  const auto& joined_return_home =
      require_prepared_i32_value_home(module,
                                      function_locations,
                                      function,
                                      join_block.terminator.value->name,
                                      "compare-join returned value");
  if (return_move.from_value_id != joined_return_home.value_id) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' compare-join return move source drifted from prepared returned value home");
  }

  bool adjusted_stack = false;
  const auto ensure_stack_frame = [&]() {
    if (!adjusted_stack && module.stack_layout.frame_size_bytes != 0) {
      function_out.append_line("    sub rsp, " +
                               std::to_string(module.stack_layout.frame_size_bytes));
      adjusted_stack = true;
    }
  };

  const auto& selected_value = return_context.selected_value;
  switch (selected_value.base.kind) {
    case c4c::backend::prepare::PreparedComputedBaseKind::ImmediateI32:
      function_out.append_line("    mov " + return_register + ", " +
                               std::to_string(selected_value.base.immediate));
      break;
    case c4c::backend::prepare::PreparedComputedBaseKind::ParamValue: {
      const auto param_name =
          c4c::backend::prepare::prepared_value_name(module.names,
                                                     selected_value.base.param_name_id);
      const auto& param_home =
          require_prepared_i32_value_home(module,
                                          function_locations,
                                          function,
                                          param_name,
                                          "compare-join selected parameter value");
      if (param_home.kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate) {
        if (!param_home.immediate_i32.has_value()) {
          throw_prepared_value_location_handoff_error(
              "defined function '" + function.name +
              "' has a rematerializable compare-join selected parameter without an immediate");
        }
        function_out.append_line("    mov " + return_register + ", " +
                                 std::to_string(*param_home.immediate_i32));
      } else {
        if (param_home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
          ensure_stack_frame();
        }
        function_out.append_line("    mov " + return_register + ", " +
                                 render_prepared_i32_value_home_operand(param_home, function));
      }
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::GlobalI32Load: {
      if (return_arm.global == nullptr ||
          find_supported_same_module_i32_global(module.module,
                                                return_arm.global->name,
                                                selected_value.base.global_byte_offset) !=
              return_arm.global) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' compare-join selected global drifted from prepared same-module global ownership");
      }
      function_out.append_line("    mov " + return_register + ", " +
                               render_global_i32_memory_operand(
                                   data,
                                   return_arm.global->name,
                                   selected_value.base.global_byte_offset));
      break;
    }
    case c4c::backend::prepare::PreparedComputedBaseKind::PointerBackedGlobalI32Load:
      if (return_arm.global == nullptr ||
          find_supported_same_module_i32_global(module.module,
                                                return_arm.global->name,
                                                selected_value.base.global_byte_offset) !=
              return_arm.global) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' compare-join pointer-backed selected global drifted from prepared same-module global ownership");
      }
      if (return_arm.pointer_root_global == nullptr ||
          return_arm.pointer_root_global->is_extern ||
          return_arm.pointer_root_global->is_thread_local ||
          return_arm.pointer_root_global->type != c4c::backend::bir::TypeKind::Ptr ||
          return_arm.pointer_root_global->size_bytes < 8 ||
          !return_arm.pointer_root_global->initializer_symbol_name.has_value() ||
          *return_arm.pointer_root_global->initializer_symbol_name != return_arm.global->name) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' compare-join pointer root drifted from prepared same-module global ownership");
      }
      function_out.append_line("    mov rax, " +
                               render_global_ptr_memory_operand(data,
                                                                return_arm.pointer_root_global->name));
      function_out.append_line("    mov " + return_register + ", " +
                               render_global_i32_memory_operand(
                                   data,
                                   return_arm.global->name,
                                   selected_value.base.global_byte_offset));
      break;
  }

  for (const auto& operation : selected_value.operations) {
    const auto mnemonic = supported_i32_binary_immediate_mnemonic(operation.opcode);
    if (!mnemonic.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an unsupported compare-join selected-value operation");
    }
    function_out.append_line("    " + std::string(*mnemonic) + " " + return_register + ", " +
                             std::to_string(operation.immediate));
  }
  if (return_context.trailing_binary.has_value()) {
    const auto mnemonic =
        supported_i32_binary_immediate_mnemonic(return_context.trailing_binary->opcode);
    if (!mnemonic.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an unsupported compare-join trailing operation");
    }
    function_out.append_line("    " + std::string(*mnemonic) + " " + return_register + ", " +
                             std::to_string(return_context.trailing_binary->immediate));
  }
  if (adjusted_stack) {
    function_out.append_line("    add rsp, " + std::to_string(module.stack_layout.frame_size_bytes));
  }
  function_out.append_line("    ret");
}

bool prepared_i32_compare_join_return_arm_is_supported(
    const c4c::backend::prepare::PreparedMaterializedCompareJoinReturnArm& return_arm) {
  switch (return_arm.shape) {
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::ImmediateI32:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::
        ImmediateI32WithTrailingImmediateBinary:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::ParamValue:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::
        ParamValueWithTrailingImmediateBinary:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::GlobalI32Load:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::
        GlobalI32LoadWithTrailingImmediateBinary:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::
        PointerBackedGlobalI32Load:
    case c4c::backend::prepare::PreparedMaterializedCompareJoinReturnShape::
        PointerBackedGlobalI32LoadWithTrailingImmediateBinary:
      return true;
  }

  std::abort();
}

bool append_prepared_i32_param_zero_compare_join_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  const auto* control_flow =
      function_name.has_value()
          ? c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                       *function_name)
          : nullptr;
  if (function.return_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  if (function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I32) {
    if (control_flow != nullptr && !control_flow->branch_conditions.empty() &&
        !control_flow->join_transfers.empty()) {
      throw_unsupported_x86_scalar_handoff_shape();
    }
    return false;
  }
  if (!function_name.has_value()) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared function id");
  }
  if (control_flow == nullptr) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto param_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names, function.params.front().name);
  if (!param_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' parameter has no prepared value id");
  }

  for (const auto& source_block : function.blocks) {
    if (source_block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }

    const auto source_block_label =
        bir_block_label_id(module.module.names, module.names, source_block);
    if (!source_block_label.has_value()) {
      throw_prepared_control_flow_handoff_error("compare-join source block '" +
                                                source_block.label +
                                                "' has no prepared block id");
    }
    if (control_flow->join_transfers.empty()) {
      const auto prepared_branch =
          c4c::backend::prepare::find_prepared_param_zero_branch_condition(module.names,
                                                                           *control_flow,
                                                                           *source_block_label,
                                                                           source_block,
                                                                           *param_name,
                                                                           false);
      if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
        continue;
      }
      const auto true_block_index = find_bir_block_index_by_prepared_label_id(
          function, module.module.names, module.names, prepared_branch->branch_condition->true_label);
      const auto false_block_index = find_bir_block_index_by_prepared_label_id(
          function, module.module.names, module.names, prepared_branch->branch_condition->false_label);
      if (true_block_index.has_value() && false_block_index.has_value() &&
          (function.blocks[*true_block_index].terminator.kind ==
               c4c::backend::bir::TerminatorKind::Branch ||
           function.blocks[*false_block_index].terminator.kind ==
               c4c::backend::bir::TerminatorKind::Branch)) {
        throw_prepared_control_flow_handoff_error(
            "compare-join source block has no authoritative prepared join metadata");
      }
      continue;
    }

    const auto prepared_branch =
        c4c::backend::prepare::find_prepared_param_zero_branch_condition(module.names,
                                                                         *control_flow,
                                                                         *source_block_label,
                                                                         source_block,
                                                                         *param_name,
                                                                         false);
    if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
      if (control_flow->branch_conditions.empty()) {
        throw_prepared_control_flow_handoff_error(
            "compare-join source block has no authoritative prepared branch metadata");
      }
      continue;
    }

    auto mutable_names = module.names;
    const auto prepared_compare_join =
        c4c::backend::prepare::find_prepared_param_zero_materialized_compare_join_branches(
            mutable_names, *control_flow, function, source_block, function.params.front(), false);
    if (!prepared_compare_join.has_value()) {
      throw_prepared_control_flow_handoff_error(
          "compare-join source block has no authoritative prepared join metadata");
    }
    const auto render_contract =
        c4c::backend::prepare::find_prepared_resolved_materialized_compare_join_render_contract(
            mutable_names, module.module, *prepared_compare_join);
    if (!render_contract.has_value()) {
      throw_prepared_control_flow_handoff_error(
          "compare-join source block has no authoritative prepared render contract");
    }
    if (!prepared_i32_compare_join_return_arm_is_supported(render_contract->true_return.arm) ||
        !prepared_i32_compare_join_return_arm_is_supported(render_contract->false_return.arm)) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an unsupported global compare-join return context");
    }

    const auto& join_context = prepared_compare_join->prepared_join_branches.compare_join_context;
    if (join_context.join_transfer == nullptr || join_context.true_transfer == nullptr ||
        join_context.false_transfer == nullptr || join_context.join_block == nullptr) {
      throw_prepared_control_flow_handoff_error(
          "compare-join source block has incomplete authoritative join metadata");
    }
    if (c4c::backend::prepare::find_prepared_control_flow_block(
            *control_flow, join_context.join_transfer->join_block_label) == nullptr) {
      throw_prepared_control_flow_handoff_error(
          "compare-join join block has no authoritative prepared control-flow block");
    }
    const auto join_block_index = find_bir_block_index_by_pointer(function, *join_context.join_block);
    if (!join_block_index.has_value()) {
      throw_prepared_control_flow_handoff_error(
          "compare-join join block is not owned by the prepared function body");
    }
    require_prepared_compare_join_parallel_copy(module.names,
                                                function,
                                                *control_flow,
                                                *function_locations,
                                                join_context.true_transfer->predecessor_label,
                                                join_context.true_transfer->successor_label);
    require_prepared_compare_join_parallel_copy(module.names,
                                                function,
                                                *control_flow,
                                                *function_locations,
                                                join_context.false_transfer->predecessor_label,
                                                join_context.false_transfer->successor_label);

    const auto symbol_name = data.render_asm_symbol_name(function.name);
    c4c::backend::x86::core::Text function_out;
    function_out.append_line(".globl " + symbol_name);
    function_out.append_line(".type " + symbol_name + ", @function");
    function_out.append_line(symbol_name + ":");

    const auto test_register = render_prepared_i32_zero_test_source(
        module, *function_locations, function, function.params.front(), function_out);
    if (test_register.has_value()) {
      function_out.append_line("    test " + *test_register + ", " + *test_register);
    }
    function_out.append_line("    " + std::string(render_contract->branch_plan.false_branch_opcode) +
                             " " + render_function_local_label(
                                       function.name,
                                       c4c::backend::prepare::prepared_block_label(
                                           module.names,
                                           render_contract->branch_plan.target_labels.false_label)));
    append_prepared_i32_compare_join_return_arm(
        function_out,
        module,
        data,
        *function_locations,
        function,
        *join_context.join_block,
        *join_block_index,
        render_contract->true_return);
    function_out.append_line(render_function_local_label(
                                 function.name,
                                 c4c::backend::prepare::prepared_block_label(
                                     module.names,
                                     render_contract->branch_plan.target_labels.false_label)) +
                             ":");
    append_prepared_i32_compare_join_return_arm(
        function_out,
        module,
        data,
        *function_locations,
        function,
        *join_context.join_block,
        *join_block_index,
        render_contract->false_return);
    out.append_raw(function_out.take_text());
    return true;
  }

  return false;
}

bool append_prepared_i32_param_zero_branch_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.return_type != c4c::backend::bir::TypeKind::I32 || function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_control_flow_handoff_error("defined function '" + function.name +
                                              "' has no prepared function id");
  }
  const auto* control_flow = c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, *function_name);
  if (control_flow == nullptr) {
    return false;
  }
  if (!control_flow->join_transfers.empty()) {
    return false;
  }

  const auto* function_locations = require_prepared_value_location_function(module, function);
  const auto param_name =
      c4c::backend::prepare::resolve_prepared_value_name_id(module.names, function.params.front().name);
  if (!param_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' parameter has no prepared value id");
  }

  for (const auto& source_block : function.blocks) {
    if (source_block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }

    const auto source_block_label =
        bir_block_label_id(module.module.names, module.names, source_block);
    if (!source_block_label.has_value()) {
      throw_prepared_control_flow_handoff_error("compare branch source block '" +
                                                source_block.label +
                                                "' has no prepared block id");
    }
    const auto prepared_branch = c4c::backend::prepare::find_prepared_param_zero_branch_condition(
        module.names,
        *control_flow,
        *source_block_label,
        source_block,
        *param_name,
        false);
    if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
      throw_prepared_control_flow_handoff_error(
          "compare branch source block has no authoritative prepared branch condition");
    }

    const auto true_block_index = find_bir_block_index_by_prepared_label_id(
        function, module.module.names, module.names, prepared_branch->branch_condition->true_label);
    const auto false_block_index = find_bir_block_index_by_prepared_label_id(
        function, module.module.names, module.names, prepared_branch->branch_condition->false_label);
    if (!true_block_index.has_value() || !false_block_index.has_value()) {
      throw_prepared_control_flow_handoff_error(
          "compare branch target block has no authoritative prepared block id");
    }
    const auto& true_block = function.blocks[*true_block_index];
    const auto& false_block = function.blocks[*false_block_index];
    if (true_block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        false_block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !true_block.terminator.value.has_value() ||
        !false_block.terminator.value.has_value() || !true_block.insts.empty() ||
        !false_block.insts.empty()) {
      return false;
    }

    const auto symbol_name = data.render_asm_symbol_name(function.name);
    c4c::backend::x86::core::Text function_out;
    function_out.append_line(".globl " + symbol_name);
    function_out.append_line(".type " + symbol_name + ", @function");
    function_out.append_line(symbol_name + ":");

    const auto test_register = render_prepared_i32_zero_test_source(
        module, *function_locations, function, function.params.front(), function_out);
    if (test_register.has_value()) {
      function_out.append_line("    test " + *test_register + ", " + *test_register);
    }
    function_out.append_line("    " + std::string(prepared_branch->false_branch_opcode) + " " +
                             render_function_local_label(function.name,
                                                         prepared_branch->false_label));
    append_prepared_i32_leaf_return(function_out,
                                    module,
                                    *function_locations,
                                    function,
                                    true_block,
                                    *true_block_index,
                                    *true_block.terminator.value);
    function_out.append_line(render_function_local_label(function.name,
                                                         prepared_branch->false_label) +
                             ":");
    append_prepared_i32_leaf_return(function_out,
                                    module,
                                    *function_locations,
                                    function,
                                    false_block,
                                    *false_block_index,
                                    *false_block.terminator.value);
    out.append_raw(function_out.take_text());
    return true;
  }

  return false;
}

const c4c::backend::prepare::PreparedMoveBundle& require_prepared_call_bundle(
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    c4c::backend::prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  const auto* bundle = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations, phase, block_index, instruction_index);
  if (bundle == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no authoritative prepared call-bundle handoff");
  }
  return *bundle;
}

std::optional<std::string> prepared_call_argument_register(
    const c4c::backend::prepare::PreparedMoveBundle& before_call_bundle,
    std::size_t arg_index) {
  for (const auto& move : before_call_bundle.moves) {
    if (move.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        move.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::Register &&
        move.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        move.destination_register_name.has_value()) {
      return *move.destination_register_name;
    }
  }
  for (const auto& binding : before_call_bundle.abi_bindings) {
    if (binding.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        binding.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::Register &&
        binding.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        binding.destination_register_name.has_value()) {
      return *binding.destination_register_name;
    }
  }
  return std::nullopt;
}

const c4c::backend::prepare::PreparedMoveResolution& require_prepared_call_result_move(
    const c4c::backend::prepare::PreparedMoveBundle& after_call_bundle,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedValueHome& result_home) {
  for (const auto& move : after_call_bundle.moves) {
    if (move.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi &&
        move.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::Register &&
        move.to_value_id == result_home.value_id && move.destination_register_name.has_value()) {
      return move;
    }
  }
  throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                              "' has no authoritative prepared call-bundle handoff");
}

bool append_prepared_direct_extern_call_argument(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const Data& data,
    const c4c::backend::bir::Value& argument,
    const std::string& destination_register) {
  if (argument.kind == c4c::backend::bir::Value::Kind::Named && !argument.name.empty() &&
      argument.name.front() == '@' && argument.type == c4c::backend::bir::TypeKind::Ptr) {
    std::string_view symbol_name(argument.name.data() + 1, argument.name.size() - 1);
    const auto names_string_constant =
        std::any_of(module.module.string_constants.begin(),
                    module.module.string_constants.end(),
                    [&](const c4c::backend::bir::StringConstant& constant) {
                      return constant.name == symbol_name;
                    });
    if (!names_string_constant) {
      return false;
    }
    function_out.append_line("    lea " + destination_register + ", [rip + " +
                             data.render_private_data_label(symbol_name) + "]");
    return true;
  }

  if (argument.kind == c4c::backend::bir::Value::Kind::Named &&
      argument.type == c4c::backend::bir::TypeKind::I32) {
    const auto& home = require_prepared_i32_value_home(
        module, function_locations, function, argument.name, "direct extern call argument");
    if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
        !home.register_name.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an unsupported prepared direct extern call argument home");
    }
    function_out.append_line("    mov " +
                             c4c::backend::x86::abi::narrow_i32_register_name(
                                 destination_register) +
                             ", " +
                             c4c::backend::x86::abi::narrow_i32_register_name(
                                 *home.register_name));
    return true;
  }

  if (argument.kind == c4c::backend::bir::Value::Kind::Immediate &&
      argument.type == c4c::backend::bir::TypeKind::I32) {
    function_out.append_line("    mov " +
                             c4c::backend::x86::abi::narrow_i32_register_name(
                                 destination_register) +
                             ", " + std::to_string(argument.immediate));
    return true;
  }

  return false;
}

bool append_prepared_direct_extern_call_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.blocks.size() != 1 || function.return_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  const auto& block = function.blocks.front();
  if (block.insts.empty() ||
      block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared function id");
  }
  const auto consumed = c4c::backend::x86::consume_plans(module, *function_name);
  if (consumed.calls == nullptr) {
    return false;
  }
  const auto* function_locations = require_prepared_value_location_function(module, function);

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");
  const std::size_t frame_adjust_bytes =
      module.stack_layout.frame_size_bytes != 0 ? module.stack_layout.frame_size_bytes : 8;
  if (frame_adjust_bytes != 0) {
    function_out.append_line("    sub rsp, " + std::to_string(frame_adjust_bytes));
  }

  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&block.insts[instruction_index]);
    if (call == nullptr || call->is_indirect ||
        call->return_type != c4c::backend::bir::TypeKind::I32 || call->inline_asm.has_value() ||
        call->sret_storage_name.has_value()) {
      return false;
    }
    const auto* call_plan =
        c4c::backend::x86::find_consumed_call_plan(consumed, 0, instruction_index);
    if (call_plan == nullptr ||
        (call_plan->wrapper_kind !=
             c4c::backend::prepare::PreparedCallWrapperKind::DirectExternFixedArity &&
         call_plan->wrapper_kind !=
             c4c::backend::prepare::PreparedCallWrapperKind::DirectExternVariadic) ||
        !call_plan->direct_callee_name.has_value() || *call_plan->direct_callee_name != call->callee) {
      return false;
    }

    const c4c::backend::prepare::PreparedMoveBundle* before_call_bundle = nullptr;
    if (!call->args.empty()) {
      before_call_bundle = &require_prepared_call_bundle(
          *function_locations,
          function,
          c4c::backend::prepare::PreparedMovePhase::BeforeCall,
          0,
          instruction_index);
    }
    for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
      const auto destination_register =
          prepared_call_argument_register(*before_call_bundle, arg_index);
      if (!destination_register.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' has no authoritative prepared call-bundle handoff");
      }
      if (!append_prepared_direct_extern_call_argument(function_out,
                                                      module,
                                                      *function_locations,
                                                      function,
                                                      data,
                                                      call->args[arg_index],
                                                      *destination_register)) {
        return false;
      }
    }

    function_out.append_line("    xor eax, eax");
    function_out.append_line("    call " + data.render_asm_symbol_name(call->callee));

    if (call->result.has_value()) {
      if (call->result->kind != c4c::backend::bir::Value::Kind::Named ||
          call->result->type != c4c::backend::bir::TypeKind::I32) {
        return false;
      }
      const auto& result_home = require_prepared_i32_value_home(
          module, *function_locations, function, call->result->name, "direct extern call result");
      if (result_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !result_home.register_name.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' has an unsupported prepared direct extern call result home");
      }
      const auto& after_call_bundle = require_prepared_call_bundle(
          *function_locations,
          function,
          c4c::backend::prepare::PreparedMovePhase::AfterCall,
          0,
          instruction_index);
      const auto& result_move =
          require_prepared_call_result_move(after_call_bundle, function, result_home);
      function_out.append_line("    mov " +
                               c4c::backend::x86::abi::narrow_i32_register_name(
                                   *result_home.register_name) +
                               ", " +
                               c4c::backend::x86::abi::narrow_i32_register_name(
                                   *result_move.destination_register_name));
    }
  }

  if (!function.return_abi.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared return ABI");
  }
  const auto return_register = c4c::backend::prepare::call_result_destination_register_name(
      c4c::backend::x86::abi::resolve_target_profile(module), *function.return_abi);
  if (!return_register.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has no prepared register return ABI destination");
  }
  function_out.append_line("    mov " +
                           c4c::backend::x86::abi::narrow_i32_register_name(*return_register) +
                           ", " + std::to_string(block.terminator.value->immediate));
  if (frame_adjust_bytes != 0) {
    function_out.append_line("    add rsp, " + std::to_string(frame_adjust_bytes));
  }
  function_out.append_line("    ret");
  for (const auto& constant : module.module.string_constants) {
    function_out.append_line(".section .rodata");
    function_out.append_line(data.render_private_data_label(constant.name) + ":");
    function_out.append_line("    .asciz \"" + escape_asm_string(constant.bytes) + "\"");
  }
  out.append_raw(function_out.take_text());
  return true;
}

struct PreparedLocalSlotCompareLoad {
  const c4c::backend::bir::LoadLocalInst* load = nullptr;
  std::size_t instruction_index = 0;
  c4c::backend::prepare::PreparedFrameSlotId frame_slot_id = 0;
  std::string memory_operand;
};

struct PreparedLocalSlotRenderedValue {
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::optional<std::string> register_name;
  std::optional<std::string> memory_operand;
};

struct PreparedLocalSlotRenderedGuardPrefix {
  std::string compare_register;
  bool saw_store = false;
};

std::optional<std::string> render_prepared_local_slot_statement_memory_operand(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind type,
    std::optional<c4c::ValueNameId> expected_result,
    std::optional<c4c::ValueNameId> expected_stored_value);

bool append_prepared_short_circuit_return_leaf(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label,
    std::size_t frame_adjust_bytes);

bool append_prepared_local_slot_scalar_guard_successor(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label,
    std::size_t frame_adjust_bytes,
    bool emit_block_label,
    std::size_t remaining_guard_depth);

std::optional<std::int64_t> prepared_branch_condition_immediate_operand(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  if (!branch_condition.lhs.has_value() || !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }
  if (branch_condition.lhs->kind == c4c::backend::bir::Value::Kind::Immediate) {
    return branch_condition.lhs->immediate;
  }
  if (branch_condition.rhs->kind == c4c::backend::bir::Value::Kind::Immediate) {
    return branch_condition.rhs->immediate;
  }
  return std::nullopt;
}

std::optional<std::string_view> prepared_branch_condition_named_operand(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  if (!branch_condition.lhs.has_value() || !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }
  if (branch_condition.lhs->kind == c4c::backend::bir::Value::Kind::Named &&
      branch_condition.rhs->kind == c4c::backend::bir::Value::Kind::Immediate) {
    return branch_condition.lhs->name;
  }
  if (branch_condition.rhs->kind == c4c::backend::bir::Value::Kind::Named &&
      branch_condition.lhs->kind == c4c::backend::bir::Value::Kind::Immediate) {
    return branch_condition.rhs->name;
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_frame_slot_memory_operand(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind type) {
  const auto* access =
      c4c::backend::prepare::find_prepared_memory_access(addressing,
                                                         block_label,
                                                         instruction_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      access->address.byte_offset < 0 ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* frame_slot =
      find_prepared_frame_slot_by_id(module.stack_layout, *access->address.frame_slot_id);
  if (frame_slot == nullptr) {
    throw_prepared_value_location_handoff_error(
        "local-slot short-circuit memory access has no prepared frame slot");
  }
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      if (access->address.size_bytes != 1) {
        return std::nullopt;
      }
      return render_stack_memory_operand(
          frame_slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset),
          "BYTE");
    case c4c::backend::bir::TypeKind::I16:
      if (access->address.size_bytes != 0 && access->address.size_bytes != 2) {
        return std::nullopt;
      }
      return render_stack_memory_operand(
          frame_slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset),
          "WORD");
    case c4c::backend::bir::TypeKind::I32:
      if (access->address.size_bytes != 4) {
        return std::nullopt;
      }
      return render_stack_memory_operand(
          frame_slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset),
          "DWORD");
    default:
      return std::nullopt;
  }
}

std::optional<PreparedLocalSlotCompareLoad> find_prepared_local_slot_compare_load(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId block_label,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  const auto loaded_value_name = prepared_branch_condition_named_operand(branch_condition);
  if (!loaded_value_name.has_value() || !branch_condition.compare_type.has_value()) {
    return std::nullopt;
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(
          module.names, function_locations, *loaded_value_name);
  if (home == nullptr) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' short-circuit compare load has no prepared home");
  }
  if (branch_condition.condition_value.kind == c4c::backend::bir::Value::Kind::Named &&
      c4c::backend::prepare::find_prepared_value_home(
          module.names, function_locations, branch_condition.condition_value.name) == nullptr) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' short-circuit compare condition has no prepared home");
  }

  for (std::size_t index = 0; index < block.insts.size(); ++index) {
    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&block.insts[index]);
    if (load == nullptr ||
        load->result.kind != c4c::backend::bir::Value::Kind::Named ||
        load->result.name != *loaded_value_name ||
        load->result.type != *branch_condition.compare_type) {
      continue;
    }
    const auto prepared_value_name =
        c4c::backend::prepare::resolve_prepared_value_name_id(module.names, load->result.name);
    if (!prepared_value_name.has_value()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' short-circuit compare load has no prepared value id");
    }
    const auto* access =
        c4c::backend::prepare::find_prepared_memory_access(addressing, block_label, index);
    const auto memory =
        render_prepared_local_slot_statement_memory_operand(module,
                                                            addressing,
                                                            block_label,
                                                            index,
                                                            load->result.type,
                                                            *prepared_value_name,
                                                            std::nullopt);
    if (!memory.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' short-circuit compare load has no prepared frame-slot access");
    }
    return PreparedLocalSlotCompareLoad{
        .load = load,
        .instruction_index = index,
        .frame_slot_id = *access->address.frame_slot_id,
        .memory_operand = *memory,
    };
  }
  for (std::size_t cast_index = 0; cast_index < block.insts.size(); ++cast_index) {
    const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&block.insts[cast_index]);
    if (cast == nullptr ||
        cast->opcode != c4c::backend::bir::CastOpcode::SExt ||
        cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
        cast->result.name != *loaded_value_name ||
        cast->result.type != *branch_condition.compare_type ||
        cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
        (cast->operand.type != c4c::backend::bir::TypeKind::I16 &&
         cast->operand.type != c4c::backend::bir::TypeKind::I8)) {
      continue;
    }
    for (std::size_t load_index = 0; load_index < cast_index; ++load_index) {
      const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&block.insts[load_index]);
      if (load == nullptr ||
          load->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load->result.name != cast->operand.name ||
          load->result.type != cast->operand.type) {
        continue;
      }
      const auto prepared_value_name =
          c4c::backend::prepare::resolve_prepared_value_name_id(module.names, load->result.name);
      if (!prepared_value_name.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' short-circuit compare load has no prepared value id");
      }
      const auto* access =
          c4c::backend::prepare::find_prepared_memory_access(addressing, block_label, load_index);
      const auto memory =
          render_prepared_local_slot_statement_memory_operand(module,
                                                              addressing,
                                                              block_label,
                                                              load_index,
                                                              load->result.type,
                                                              *prepared_value_name,
                                                              std::nullopt);
      if (!memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' short-circuit compare load has no prepared frame-slot access");
      }
      return PreparedLocalSlotCompareLoad{
          .load = load,
          .instruction_index = load_index,
          .frame_slot_id = *access->address.frame_slot_id,
          .memory_operand = *memory,
      };
    }
  }
  return std::nullopt;
}

bool block_uses_local_slot_memory(const c4c::backend::bir::Block& block) {
  return std::any_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
    return std::holds_alternative<c4c::backend::bir::StoreLocalInst>(inst) ||
           std::holds_alternative<c4c::backend::bir::LoadLocalInst>(inst);
  });
}

std::optional<std::string> render_prepared_local_slot_statement_memory_operand(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind type,
    std::optional<c4c::ValueNameId> expected_result,
    std::optional<c4c::ValueNameId> expected_stored_value) {
  const auto* access =
      c4c::backend::prepare::find_prepared_memory_access(addressing, block_label, instruction_index);
  const auto memory =
      render_prepared_frame_slot_memory_operand(module, addressing, block_label, instruction_index, type);
  if (!memory.has_value()) {
    return std::nullopt;
  }
  if (access == nullptr || access->result_value_name != expected_result ||
      access->stored_value_name != expected_stored_value) {
    throw_prepared_value_location_handoff_error(
        "local-slot statement drifted from prepared frame-slot access");
  }
  return memory;
}

std::optional<std::size_t> find_prepared_local_slot_compare_instruction_index(
    const c4c::backend::bir::Block& block,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition) {
  if (branch_condition.condition_value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < block.insts.size(); ++index) {
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index]);
    if (binary != nullptr &&
        (binary->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
         binary->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
        binary->result.kind == c4c::backend::bir::Value::Kind::Named &&
        binary->result.name == branch_condition.condition_value.name) {
      return index;
    }
  }
  return std::nullopt;
}

std::optional<std::string> prepared_local_slot_named_i32_operand_register(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    std::unordered_map<std::string_view, PreparedLocalSlotRenderedValue>& values,
    std::string_view value_name,
    std::string_view context) {
  const auto value = values.find(value_name);
  if (value == values.end() || value->second.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value->second.register_name.has_value()) {
    return *value->second.register_name;
  }
  if (!value->second.memory_operand.has_value()) {
    return std::nullopt;
  }
  const auto register_name =
      require_prepared_i32_register_home(module, function_locations, function, value_name, context);
  function_out.append_line("    mov " + register_name + ", " + *value->second.memory_operand);
  value->second.register_name = register_name;
  return register_name;
}

std::optional<PreparedLocalSlotRenderedGuardPrefix> render_prepared_local_slot_i32_guard_prefix(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& block,
    c4c::BlockLabelId block_label,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    bool require_binary_expression) {
  if (!branch_condition.compare_type.has_value() ||
      *branch_condition.compare_type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto compare_index =
      find_prepared_local_slot_compare_instruction_index(block, branch_condition);
  if (!compare_index.has_value()) {
    return std::nullopt;
  }
  const auto* compare_binary =
      std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[*compare_index]);
  if (compare_binary == nullptr ||
      compare_binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
      compare_binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      compare_binary->result.type != c4c::backend::bir::TypeKind::I32 ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  const auto compare_named_operand =
      prepared_branch_condition_named_operand(branch_condition);
  if (!compare_named_operand.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, PreparedLocalSlotRenderedValue> values;
  bool saw_store = false;
  bool saw_binary = false;
  for (std::size_t index = 0; index < *compare_index; ++index) {
    if (const auto* store =
            std::get_if<c4c::backend::bir::StoreLocalInst>(&block.insts[index])) {
      std::optional<c4c::ValueNameId> stored_value;
      std::string source_operand;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I32) {
        source_operand = std::to_string(store->value.immediate);
      } else if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
                 store->value.type == c4c::backend::bir::TypeKind::I32) {
        stored_value =
            c4c::backend::prepare::resolve_prepared_value_name_id(module.names, store->value.name);
        if (!stored_value.has_value()) {
          throw_prepared_value_location_handoff_error(
              "defined function '" + function.name +
              "' local-slot guard store has no prepared value id");
        }
        auto source_register = prepared_local_slot_named_i32_operand_register(
            function_out,
            module,
            function_locations,
            function,
            values,
            store->value.name,
            "local-slot guard stored value");
        if (!source_register.has_value()) {
          return std::nullopt;
        }
        source_operand = *source_register;
      } else {
        return std::nullopt;
      }

      const auto memory = render_prepared_local_slot_statement_memory_operand(module,
                                                                              addressing,
                                                                              block_label,
                                                                              index,
                                                                              c4c::backend::bir::TypeKind::I32,
                                                                              std::nullopt,
                                                                              stored_value);
      if (!memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot guard store has no prepared frame-slot access");
      }
      saw_store = true;
      function_out.append_line("    mov " + *memory + ", " + source_operand);
      continue;
    }

    if (const auto* load =
            std::get_if<c4c::backend::bir::LoadLocalInst>(&block.insts[index])) {
      if (load->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load->result.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto prepared_value_name =
          c4c::backend::prepare::resolve_prepared_value_name_id(module.names, load->result.name);
      if (!prepared_value_name.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name + "' local-slot guard load has no prepared value id");
      }
      const auto memory = render_prepared_local_slot_statement_memory_operand(module,
                                                                              addressing,
                                                                              block_label,
                                                                              index,
                                                                              load->result.type,
                                                                              *prepared_value_name,
                                                                              std::nullopt);
      if (!memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot guard load has no prepared frame-slot access");
      }
      values[load->result.name] = PreparedLocalSlotRenderedValue{
          .type = load->result.type,
          .register_name = std::nullopt,
          .memory_operand = *memory,
      };
      continue;
    }

    if (const auto* binary =
            std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index])) {
      if (binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
          binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
          binary->result.type != c4c::backend::bir::TypeKind::I32 ||
          binary->lhs.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto mnemonic = supported_i32_binary_immediate_mnemonic(binary->opcode);
      if (!mnemonic.has_value()) {
        return std::nullopt;
      }
      auto lhs_register = prepared_local_slot_named_i32_operand_register(function_out,
                                                                         module,
                                                                         function_locations,
                                                                         function,
                                                                         values,
                                                                         binary->lhs.name,
                                                                         "local-slot guard binary lhs");
      if (!lhs_register.has_value()) {
        return std::nullopt;
      }
      const auto result_register =
          require_prepared_i32_register_home(module,
                                             function_locations,
                                             function,
                                             binary->result.name,
                                             "local-slot guard binary result value");
      if (*lhs_register != result_register) {
        function_out.append_line("    mov " + result_register + ", " + *lhs_register);
      }

      std::string rhs_operand;
      if (binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
        rhs_operand = std::to_string(binary->rhs.immediate);
      } else if (binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                 binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
        const auto rhs_value = values.find(binary->rhs.name);
        if (rhs_value == values.end()) {
          return std::nullopt;
        }
        if (rhs_value->second.memory_operand.has_value()) {
          rhs_operand = *rhs_value->second.memory_operand;
        } else if (rhs_value->second.register_name.has_value()) {
          rhs_operand = *rhs_value->second.register_name;
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
      function_out.append_line("    " + std::string(*mnemonic) + " " + result_register +
                               ", " + rhs_operand);
      values[binary->result.name] = PreparedLocalSlotRenderedValue{
          .type = binary->result.type,
          .register_name = result_register,
          .memory_operand = std::nullopt,
      };
      saw_binary = true;
      continue;
    }

    return std::nullopt;
  }

  auto compare_register = prepared_local_slot_named_i32_operand_register(function_out,
                                                                         module,
                                                                         function_locations,
                                                                         function,
                                                                         values,
                                                                         *compare_named_operand,
                                                                         "local-slot guard compare value");
  if ((require_binary_expression && !saw_binary) || !compare_register.has_value()) {
    return std::nullopt;
  }
  return PreparedLocalSlotRenderedGuardPrefix{
      .compare_register = *compare_register,
      .saw_store = saw_store,
  };
}

bool append_prepared_local_slot_return_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.return_type != c4c::backend::bir::TypeKind::I32 ||
      function.blocks.size() != 1) {
    return false;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    return false;
  }
  const auto* function_locations =
      c4c::backend::prepare::find_prepared_value_location_function(module, *function_name);
  const auto* addressing =
      c4c::backend::prepare::find_prepared_addressing(module, *function_name);
  if (function_locations == nullptr || addressing == nullptr) {
    return false;
  }

  const auto& block = function.blocks.front();
  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      block.terminator.value->type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  const auto block_label = bir_block_label_id(module.module.names, module.names, block);
  if (!block_label.has_value()) {
    return false;
  }

  const auto frame_adjust_bytes =
      normalize_x86_local_frame_adjust(addressing->frame_size_bytes != 0
                                           ? addressing->frame_size_bytes
                                           : module.stack_layout.frame_size_bytes);
  if (frame_adjust_bytes == 0) {
    return false;
  }

  if (std::none_of(block.insts.begin(), block.insts.end(), [](const auto& inst) {
        return std::holds_alternative<c4c::backend::bir::StoreLocalInst>(inst) ||
               std::holds_alternative<c4c::backend::bir::LoadLocalInst>(inst);
      })) {
    return false;
  }

  const auto& return_home =
      require_prepared_i32_value_home(module,
                                      *function_locations,
                                      function,
                                      block.terminator.value->name,
                                      "local-slot return value");
  const auto& return_move =
      require_prepared_i32_return_move(*function_locations, function, block, 0);
  if (return_move.from_value_id != return_home.value_id) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' local-slot return move source drifted from value home");
  }
  if (return_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      !return_home.register_name.has_value()) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' has an unsupported prepared local-slot return home");
  }
  const auto return_register =
      c4c::backend::x86::abi::narrow_i32_register_name(*return_move.destination_register_name);

  std::unordered_map<std::string_view, std::string> loaded_values;
  bool saw_local_slot_statement = false;
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");
  function_out.append_line("    sub rsp, " + std::to_string(frame_adjust_bytes));

  for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
    if (const auto* store =
            std::get_if<c4c::backend::bir::StoreLocalInst>(&block.insts[inst_index])) {
      if (store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return false;
      }
      const auto memory = render_prepared_local_slot_statement_memory_operand(
          module,
          *addressing,
          *block_label,
          inst_index,
          store->value.type,
          std::nullopt,
          std::nullopt);
      if (!memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot store has no prepared frame-slot access");
      }
      saw_local_slot_statement = true;
      function_out.append_line("    mov " + *memory + ", " +
                               std::to_string(store->value.immediate));
      continue;
    }

    if (const auto* load =
            std::get_if<c4c::backend::bir::LoadLocalInst>(&block.insts[inst_index])) {
      if (load->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load->result.type != c4c::backend::bir::TypeKind::I32) {
        return false;
      }
      const auto prepared_value_name =
          c4c::backend::prepare::resolve_prepared_value_name_id(module.names, load->result.name);
      if (!prepared_value_name.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' local-slot load has no prepared value id");
      }
      const auto& load_home = require_prepared_i32_value_home(
          module, *function_locations, function, load->result.name, "local-slot load result value");
      const auto memory = render_prepared_local_slot_statement_memory_operand(
          module,
          *addressing,
          *block_label,
          inst_index,
          load->result.type,
          *prepared_value_name,
          std::nullopt);
      if (!memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot load has no prepared frame-slot access");
      }
      if (load_home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !load_home.register_name.has_value()) {
        throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                    "' has an unsupported prepared local-slot load home");
      }
      saw_local_slot_statement = true;
      const auto load_register = load->result.name == block.terminator.value->name
                                     ? return_register
                                     : c4c::backend::x86::abi::narrow_i32_register_name(
                                           *load_home.register_name);
      function_out.append_line("    mov " + load_register + ", " + *memory);
      loaded_values[load->result.name] = load_register;
      continue;
    }

    return false;
  }

  if (!saw_local_slot_statement) {
    return false;
  }
  const auto returned_value = loaded_values.find(block.terminator.value->name);
  if (returned_value == loaded_values.end()) {
    return false;
  }
  if (returned_value->second != return_register) {
    function_out.append_line("    mov " + return_register + ", " + returned_value->second);
  }
  function_out.append_line("    add rsp, " + std::to_string(frame_adjust_bytes));
  function_out.append_line("    ret");
  out.append_raw(function_out.take_text());
  return true;
}

bool append_prepared_local_slot_scalar_guard_block(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& guard_block,
    c4c::BlockLabelId guard_label,
    std::size_t frame_adjust_bytes,
    bool require_store,
    bool emit_block_label,
    std::size_t remaining_guard_depth) {
  const auto* guard_condition =
      c4c::backend::prepare::find_prepared_branch_condition(control_flow, guard_label);
  if (guard_condition == nullptr ||
      !guard_condition->predicate.has_value() ||
      !guard_condition->compare_type.has_value() ||
      !guard_condition->lhs.has_value() ||
      !guard_condition->rhs.has_value() ||
      *guard_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
      (*guard_condition->predicate != c4c::backend::bir::BinaryOpcode::Eq &&
       *guard_condition->predicate != c4c::backend::bir::BinaryOpcode::Ne)) {
    return false;
  }

  auto mutable_names = module.names;
  validate_prepared_branch_condition(function,
                                     module.module.names,
                                     mutable_names,
                                     control_flow,
                                     *guard_condition);

  const auto branch_targets =
      c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
          module.names, &control_flow, guard_label, guard_block, *guard_condition);
  if (!branch_targets.has_value()) {
    throw_prepared_control_flow_handoff_error(
        "prepared local-slot guard targets drifted from prepared block targets");
  }

  const auto immediate = prepared_branch_condition_immediate_operand(*guard_condition);
  if (!immediate.has_value()) {
    return false;
  }

  const auto condition_home =
      guard_condition->condition_value.kind == c4c::backend::bir::Value::Kind::Named
          ? c4c::backend::prepare::find_prepared_value_home(module.names,
                                                            function_locations,
                                                            guard_condition->condition_value.name)
          : nullptr;
  if (condition_home == nullptr) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name + "' local-slot guard condition has no prepared home");
  }

  const auto compare_load =
      find_prepared_local_slot_compare_load(module,
                                            addressing,
                                            function_locations,
                                            function,
                                            guard_block,
                                            guard_label,
                                            *guard_condition);
  if (!compare_load.has_value()) {
    return false;
  }

  const auto entry_type = compare_load->load->result.type;
  if (entry_type != c4c::backend::bir::TypeKind::I32 &&
      entry_type != c4c::backend::bir::TypeKind::I16 &&
      entry_type != c4c::backend::bir::TypeKind::I8) {
    return false;
  }

  if (emit_block_label) {
    const auto block_label_text =
        c4c::backend::prepare::prepared_block_label(module.names, guard_label);
    function_out.append_line(render_function_local_label(function.name, block_label_text) + ":");
  }

  bool saw_store = false;
  std::unordered_map<std::string_view, std::string> scalar_registers;
  for (std::size_t index = 0; index < compare_load->instruction_index; ++index) {
    if (const auto* store =
            std::get_if<c4c::backend::bir::StoreLocalInst>(&guard_block.insts[index])) {
      std::optional<c4c::ValueNameId> stored_value;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named) {
        stored_value =
            c4c::backend::prepare::resolve_prepared_value_name_id(module.names, store->value.name);
        if (!stored_value.has_value()) {
          throw_prepared_value_location_handoff_error(
              "defined function '" + function.name + "' local-slot guard store has no prepared value id");
        }
      }
      if (store->value.type != entry_type ||
          (store->value.kind != c4c::backend::bir::Value::Kind::Immediate &&
           store->value.kind != c4c::backend::bir::Value::Kind::Named)) {
        return false;
      }
      const auto store_memory =
          render_prepared_local_slot_statement_memory_operand(module,
                                                              addressing,
                                                              guard_label,
                                                              index,
                                                              entry_type,
                                                              std::nullopt,
                                                              stored_value);
      if (!store_memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot guard store drifted from prepared frame-slot access");
      }
      saw_store = true;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
        function_out.append_line("    mov " + *store_memory + ", " +
                                 std::to_string(store->value.immediate));
      } else {
        const auto stored_register = scalar_registers.find(store->value.name);
        if (stored_register == scalar_registers.end()) {
          return false;
        }
        if (entry_type == c4c::backend::bir::TypeKind::I32) {
          function_out.append_line("    mov " + *store_memory + ", " + stored_register->second);
        } else {
          const auto stored_narrow_register =
              entry_type == c4c::backend::bir::TypeKind::I8
                  ? narrow_i8_register_name(stored_register->second)
                  : narrow_i16_register_name(stored_register->second);
          function_out.append_line("    mov " + *store_memory + ", " + stored_narrow_register);
        }
      }
      continue;
    }

    if (const auto* load =
            std::get_if<c4c::backend::bir::LoadLocalInst>(&guard_block.insts[index])) {
      if (load->result.kind != c4c::backend::bir::Value::Kind::Named ||
          load->result.type != entry_type) {
        return false;
      }
      const auto prepared_value_name =
          c4c::backend::prepare::resolve_prepared_value_name_id(module.names, load->result.name);
      if (!prepared_value_name.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name + "' local-slot guard load has no prepared value id");
      }
      const auto load_memory =
          render_prepared_local_slot_statement_memory_operand(module,
                                                              addressing,
                                                              guard_label,
                                                              index,
                                                              load->result.type,
                                                              *prepared_value_name,
                                                              std::nullopt);
      if (!load_memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot guard load drifted from prepared frame-slot access");
      }
      if (entry_type == c4c::backend::bir::TypeKind::I32) {
        function_out.append_line("    mov eax, " + *load_memory);
      } else {
        function_out.append_line("    movsx eax, " + *load_memory);
      }
      scalar_registers[load->result.name] = "eax";
      continue;
    }

    if (const auto* cast =
            std::get_if<c4c::backend::bir::CastInst>(&guard_block.insts[index])) {
      if (cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
          cast->operand.kind != c4c::backend::bir::Value::Kind::Named) {
        return false;
      }
      const auto source_register = scalar_registers.find(cast->operand.name);
      if (source_register == scalar_registers.end()) {
        return false;
      }
      if (entry_type != c4c::backend::bir::TypeKind::I32 &&
          cast->opcode == c4c::backend::bir::CastOpcode::SExt &&
          cast->operand.type == entry_type &&
          cast->result.type == c4c::backend::bir::TypeKind::I32) {
        const auto result_register =
            require_prepared_i32_register_home(module,
                                               function_locations,
                                               function,
                                               cast->result.name,
                                               "local-slot guard sext result value");
        if (result_register != source_register->second) {
          function_out.append_line("    mov " + result_register + ", " + source_register->second);
        }
        scalar_registers[cast->result.name] = result_register;
        continue;
      }
      if (entry_type != c4c::backend::bir::TypeKind::I32 &&
          cast->opcode == c4c::backend::bir::CastOpcode::Trunc &&
          cast->operand.type == c4c::backend::bir::TypeKind::I32 &&
          cast->result.type == entry_type) {
        scalar_registers[cast->result.name] = source_register->second;
        continue;
      }
      return false;
    }

    if (const auto* binary =
            std::get_if<c4c::backend::bir::BinaryInst>(&guard_block.insts[index])) {
      if (entry_type != c4c::backend::bir::TypeKind::I16 ||
          binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
          binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
          binary->result.type != c4c::backend::bir::TypeKind::I32 ||
          binary->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
          binary->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
          binary->rhs.type != c4c::backend::bir::TypeKind::I32) {
        return false;
      }
      const auto mnemonic = supported_i32_binary_immediate_mnemonic(binary->opcode);
      if (!mnemonic.has_value()) {
        return false;
      }
      const auto source_register = scalar_registers.find(binary->lhs.name);
      if (source_register == scalar_registers.end()) {
        return false;
      }
      const auto result_register =
          require_prepared_i32_register_home(module,
                                             function_locations,
                                             function,
                                             binary->result.name,
                                             "local-slot guard binary result value");
      if (result_register != source_register->second) {
        function_out.append_line("    mov " + result_register + ", " + source_register->second);
      }
      function_out.append_line("    " + std::string(*mnemonic) + " " + result_register + ", " +
                               std::to_string(binary->rhs.immediate));
      scalar_registers[binary->result.name] = result_register;
      continue;
    }

    return false;
  }
  if (require_store && !saw_store) {
    return false;
  }

  if (entry_type == c4c::backend::bir::TypeKind::I32) {
    function_out.append_line("    mov eax, " + compare_load->memory_operand);
  } else {
    function_out.append_line("    movsx eax, " + compare_load->memory_operand);
  }
  function_out.append_line("    cmp eax, " + std::to_string(*immediate));

  const auto jump_on_equal =
      *guard_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq
          ? branch_targets->true_label
          : branch_targets->false_label;
  const auto fallthrough_label =
      *guard_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq
          ? branch_targets->false_label
          : branch_targets->true_label;
  const auto jump_label_text =
      c4c::backend::prepare::prepared_block_label(module.names, jump_on_equal);
  function_out.append_line("    je " +
                           render_function_local_label(function.name, jump_label_text));

  if (!append_prepared_local_slot_scalar_guard_successor(function_out,
                                                         module,
                                                         addressing,
                                                         function_locations,
                                                         control_flow,
                                                         function,
                                                         fallthrough_label,
                                                         frame_adjust_bytes,
                                                         false,
                                                         remaining_guard_depth)) {
    return false;
  }

  if (!append_prepared_local_slot_scalar_guard_successor(function_out,
                                                         module,
                                                         addressing,
                                                         function_locations,
                                                         control_flow,
                                                         function,
                                                         jump_on_equal,
                                                         frame_adjust_bytes,
                                                         true,
                                                         remaining_guard_depth)) {
    return false;
  }

  return true;
}

bool append_prepared_local_slot_scalar_guard_successor(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedAddressingFunction& addressing,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label,
    std::size_t frame_adjust_bytes,
    bool emit_block_label,
    std::size_t remaining_guard_depth) {
  if (emit_block_label) {
    const auto block_label_text =
        c4c::backend::prepare::prepared_block_label(module.names, block_label);
    function_out.append_line(render_function_local_label(function.name, block_label_text) + ":");
  }
  if (append_prepared_short_circuit_return_leaf(function_out,
                                                module,
                                                function_locations,
                                                function,
                                                block_label,
                                                frame_adjust_bytes)) {
    return true;
  }
  if (remaining_guard_depth == 0) {
    return false;
  }

  const auto* block =
      find_bir_block_by_prepared_label(function, module.module.names, module.names, block_label);
  if (block == nullptr ||
      block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      !block_uses_local_slot_memory(*block)) {
    return false;
  }

  return append_prepared_local_slot_scalar_guard_block(function_out,
                                                       module,
                                                       addressing,
                                                       function_locations,
                                                       control_flow,
                                                       function,
                                                       *block,
                                                       block_label,
                                                       frame_adjust_bytes,
                                                       false,
                                                       false,
                                                       remaining_guard_depth - 1);
}

bool append_prepared_local_slot_immediate_guard_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.return_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    return false;
  }
  const auto* control_flow =
      c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                 *function_name);
  const auto* function_locations =
      c4c::backend::prepare::find_prepared_value_location_function(module, *function_name);
  const auto* addressing =
      c4c::backend::prepare::find_prepared_addressing(module, *function_name);
  if (control_flow == nullptr || function_locations == nullptr || addressing == nullptr) {
    return false;
  }
  if (!control_flow->join_transfers.empty()) {
    return false;
  }

  const c4c::backend::bir::Block* guard_block = nullptr;
  c4c::BlockLabelId guard_label = c4c::kInvalidBlockLabel;
  const c4c::backend::prepare::PreparedBranchCondition* guard_condition = nullptr;
  for (const auto& block : function.blocks) {
    if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }
    const auto block_label = bir_block_label_id(module.module.names, module.names, block);
    if (!block_label.has_value()) {
      continue;
    }
    const auto* condition =
        c4c::backend::prepare::find_prepared_branch_condition(*control_flow, *block_label);
    if (condition == nullptr) {
      if (block_uses_local_slot_memory(block)) {
        throw_prepared_control_flow_handoff_error(
            "local-slot guard source block has no authoritative prepared branch metadata");
      }
      continue;
    }
    if (!block_uses_local_slot_memory(block)) {
      continue;
    }
    guard_block = &block;
    guard_label = *block_label;
    guard_condition = condition;
    break;
  }

  if (guard_block == nullptr || guard_condition == nullptr) {
    return false;
  }

  if (!guard_condition->predicate.has_value() ||
      !guard_condition->compare_type.has_value() ||
      !guard_condition->lhs.has_value() ||
      !guard_condition->rhs.has_value() ||
      *guard_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
      (*guard_condition->predicate != c4c::backend::bir::BinaryOpcode::Eq &&
       *guard_condition->predicate != c4c::backend::bir::BinaryOpcode::Ne)) {
    return false;
  }

  auto mutable_names = module.names;
  validate_prepared_branch_condition(function,
                                     module.module.names,
                                     mutable_names,
                                     *control_flow,
                                     *guard_condition);

  const auto branch_targets =
      c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
          module.names, control_flow, guard_label, *guard_block, *guard_condition);
  if (!branch_targets.has_value()) {
    throw_prepared_control_flow_handoff_error(
        "prepared local-slot guard targets drifted from prepared block targets");
  }

  const auto immediate = prepared_branch_condition_immediate_operand(*guard_condition);
  if (!immediate.has_value()) {
    return false;
  }

  const auto condition_home =
      guard_condition->condition_value.kind == c4c::backend::bir::Value::Kind::Named
          ? c4c::backend::prepare::find_prepared_value_home(module.names,
                                                            *function_locations,
                                                            guard_condition->condition_value.name)
          : nullptr;
  if (condition_home == nullptr) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name + "' local-slot guard condition has no prepared home");
  }

  const auto frame_adjust_bytes =
      normalize_x86_local_frame_adjust(addressing->frame_size_bytes != 0
                                           ? addressing->frame_size_bytes
                                           : module.stack_layout.frame_size_bytes);
  if (frame_adjust_bytes == 0) {
    throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                "' local-slot guard has no prepared frame size");
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");
  function_out.append_line("    sub rsp, " + std::to_string(frame_adjust_bytes));

  c4c::backend::x86::core::Text rendered_i32_prefix_text;
  bool rendered_i32_predecessor_store = false;
  for (const auto& predecessor_block : function.blocks) {
    if (predecessor_block.terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
      continue;
    }
    const auto predecessor_label =
        bir_block_label_id(module.module.names, module.names, predecessor_block);
    if (!predecessor_label.has_value()) {
      continue;
    }
    const auto* prepared_predecessor =
        c4c::backend::prepare::find_prepared_control_flow_block(*control_flow,
                                                                 *predecessor_label);
    if (prepared_predecessor == nullptr ||
        prepared_predecessor->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
        prepared_predecessor->branch_target_label != guard_label) {
      continue;
    }
    for (std::size_t index = 0; index < predecessor_block.insts.size(); ++index) {
      const auto* store =
          std::get_if<c4c::backend::bir::StoreLocalInst>(&predecessor_block.insts[index]);
      if (store == nullptr ||
          store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        rendered_i32_predecessor_store = false;
        break;
      }
      const auto store_memory =
          render_prepared_local_slot_statement_memory_operand(module,
                                                              *addressing,
                                                              *predecessor_label,
                                                              index,
                                                              store->value.type,
                                                              std::nullopt,
                                                              std::nullopt);
      if (!store_memory.has_value()) {
        throw_prepared_value_location_handoff_error(
            "defined function '" + function.name +
            "' local-slot branch predecessor store has no prepared frame-slot access");
      }
      rendered_i32_prefix_text.append_line("    mov " + *store_memory + ", " +
                                           std::to_string(store->value.immediate));
      rendered_i32_predecessor_store = true;
    }
  }
  const auto rendered_i32_prefix =
      render_prepared_local_slot_i32_guard_prefix(rendered_i32_prefix_text,
                                                  module,
                                                  *addressing,
                                                  *function_locations,
                                                  function,
                                                  *guard_block,
                                                  guard_label,
                                                  *guard_condition,
                                                  !rendered_i32_predecessor_store);
  if (rendered_i32_prefix.has_value()) {
    if (!rendered_i32_prefix->saw_store && !rendered_i32_predecessor_store) {
      return false;
    }
    function_out.append_raw(rendered_i32_prefix_text.take_text());
    function_out.append_line("    cmp " + rendered_i32_prefix->compare_register + ", " +
                             std::to_string(*immediate));

    const auto jump_on_equal =
        *guard_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq
            ? branch_targets->true_label
            : branch_targets->false_label;
    const auto fallthrough_label =
        *guard_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq
            ? branch_targets->false_label
            : branch_targets->true_label;
    const auto jump_label_text =
        c4c::backend::prepare::prepared_block_label(module.names, jump_on_equal);
    function_out.append_line("    je " +
                             render_function_local_label(function.name, jump_label_text));

    if (!append_prepared_short_circuit_return_leaf(function_out,
                                                  module,
                                                  *function_locations,
                                                  function,
                                                  fallthrough_label,
                                                  frame_adjust_bytes)) {
      return false;
    }

    function_out.append_line(render_function_local_label(function.name, jump_label_text) + ":");
    if (!append_prepared_short_circuit_return_leaf(function_out,
                                                  module,
                                                  *function_locations,
                                                  function,
                                                  jump_on_equal,
                                                  frame_adjust_bytes)) {
      return false;
    }

    out.append_raw(function_out.take_text());
    return true;
  }

  if (!append_prepared_local_slot_scalar_guard_block(function_out,
                                                     module,
                                                     *addressing,
                                                     *function_locations,
                                                     *control_flow,
                                                     function,
                                                     *guard_block,
                                                     guard_label,
                                                     frame_adjust_bytes,
                                                     true,
                                                     false,
                                                     function.blocks.size())) {
    return false;
  }

  out.append_raw(function_out.take_text());
  return true;
}

bool append_prepared_short_circuit_return_leaf(
    c4c::backend::x86::core::Text& function_out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId block_label,
    std::size_t frame_adjust_bytes) {
  const auto* block =
      find_bir_block_by_prepared_label(function, module.module.names, module.names, block_label);
  const auto block_index =
      block != nullptr ? find_bir_block_index_by_pointer(function, *block) : std::nullopt;
  if (block == nullptr || !block_index.has_value() ||
      block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !block->terminator.value.has_value() ||
      block->terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
      block->terminator.value->type != c4c::backend::bir::TypeKind::I32 ||
      (block->terminator.value->immediate != 0 && block->terminator.value->immediate != 1)) {
    return false;
  }

  const auto* before_return = c4c::backend::prepare::find_prepared_move_bundle(
      function_locations,
      c4c::backend::prepare::PreparedMovePhase::BeforeReturn,
      *block_index,
      block->insts.size());
  std::optional<std::string> return_register_name;
  if (before_return != nullptr) {
    if (before_return->moves.size() != 1) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an ambiguous prepared short-circuit return move bundle");
    }
    const auto& return_move = before_return->moves.front();
    if (return_move.destination_kind !=
            c4c::backend::prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
        return_move.destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::Register ||
        !return_move.destination_register_name.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has an unsupported prepared short-circuit return ABI move");
    }
    if (return_move.source_immediate_i32.has_value() &&
        *return_move.source_immediate_i32 != block->terminator.value->immediate) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' short-circuit return immediate drifted from prepared return move");
    }
    return_register_name = *return_move.destination_register_name;
  } else {
    if (!function.return_abi.has_value()) {
      throw_prepared_value_location_handoff_error("defined function '" + function.name +
                                                  "' has no prepared return ABI");
    }
    return_register_name = c4c::backend::prepare::call_result_destination_register_name(
        c4c::backend::x86::abi::resolve_target_profile(module), *function.return_abi);
    if (!return_register_name.has_value()) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' has no prepared short-circuit register return ABI destination");
    }
  }
  function_out.append_line("    mov " +
                           c4c::backend::x86::abi::narrow_i32_register_name(*return_register_name) +
                           ", " + std::to_string(block->terminator.value->immediate));
  if (frame_adjust_bytes != 0) {
    function_out.append_line("    add rsp, " + std::to_string(frame_adjust_bytes));
  }
  function_out.append_line("    ret");
  return true;
}

bool append_prepared_local_slot_short_circuit_or_guard_function(
    c4c::backend::x86::core::Text& out,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::bir::Function& function,
    const Data& data) {
  if (function.return_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  const auto function_name =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
  if (!function_name.has_value()) {
    return false;
  }
  const auto* control_flow =
      c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                 *function_name);
  const auto* function_locations =
      c4c::backend::prepare::find_prepared_value_location_function(module, *function_name);
  const auto* addressing =
      c4c::backend::prepare::find_prepared_addressing(module, *function_name);
  if (control_flow == nullptr || function_locations == nullptr || addressing == nullptr ||
      control_flow->join_transfers.size() != 1) {
    return false;
  }

  const c4c::backend::bir::Block* entry_block = nullptr;
  c4c::BlockLabelId entry_label = c4c::kInvalidBlockLabel;
  const c4c::backend::prepare::PreparedBranchCondition* entry_condition = nullptr;
  std::optional<c4c::backend::prepare::PreparedShortCircuitJoinContext> join_context;
  for (const auto& block : function.blocks) {
    if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      continue;
    }
    const auto block_label =
        bir_block_label_id(module.module.names, module.names, block);
    if (!block_label.has_value()) {
      continue;
    }
    auto candidate_context =
        c4c::backend::prepare::find_prepared_short_circuit_join_context(
            module.names, *control_flow, function, *block_label);
    if (!candidate_context.has_value()) {
      continue;
    }
    const auto* candidate_condition =
        c4c::backend::prepare::find_prepared_branch_condition(*control_flow, *block_label);
    if (candidate_condition == nullptr) {
      return false;
    }
    entry_block = &block;
    entry_label = *block_label;
    entry_condition = candidate_condition;
    join_context = std::move(candidate_context);
    break;
  }
  if (entry_block == nullptr || entry_condition == nullptr || !join_context.has_value() ||
      join_context->true_transfer == nullptr || join_context->false_transfer == nullptr) {
    return false;
  }

  const auto authoritative_bundles =
      c4c::backend::prepare::find_authoritative_branch_owned_parallel_copy_bundles(
          module.names,
          *control_flow,
          entry_label,
          c4c::backend::prepare::PreparedJoinTransferKind::PhiEdge,
          join_context->true_transfer->predecessor_label,
          join_context->false_transfer->predecessor_label);
  if (!authoritative_bundles.has_value() ||
      authoritative_bundles->true_bundle == nullptr ||
      authoritative_bundles->false_bundle == nullptr) {
    throw_prepared_control_flow_handoff_error(
        "defined function '" + function.name +
        "' has no authoritative prepared short-circuit parallel-copy bundles");
  }
  if (c4c::backend::prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
          module.names, function, *function_locations, *authoritative_bundles->true_bundle) ==
          nullptr ||
      c4c::backend::prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
          module.names, function, *function_locations, *authoritative_bundles->false_bundle) ==
          nullptr) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' has no prepared short-circuit out-of-SSA move bundles");
  }

  const auto direct_targets =
      c4c::backend::prepare::find_prepared_compare_branch_target_labels(
          module.names, *entry_condition, *entry_block);
  if (!direct_targets.has_value()) {
    return false;
  }
  const auto branch_plan =
      c4c::backend::prepare::find_prepared_short_circuit_branch_plan(
          module.names, *join_context, *direct_targets);
  if (!branch_plan.has_value()) {
    return false;
  }

  const auto entry_compare =
      find_prepared_local_slot_compare_load(module,
                                            *addressing,
                                            *function_locations,
                                            function,
                                            *entry_block,
                                            entry_label,
                                            *entry_condition);
  if (!entry_compare.has_value()) {
    return false;
  }
  const auto* rhs_target = branch_plan->on_compare_true.continuation.has_value()
                               ? &branch_plan->on_compare_true
                               : branch_plan->on_compare_false.continuation.has_value()
                                     ? &branch_plan->on_compare_false
                                     : nullptr;
  const auto* short_circuit_target = rhs_target == &branch_plan->on_compare_true
                                         ? &branch_plan->on_compare_false
                                         : &branch_plan->on_compare_true;
  if (rhs_target == nullptr || !rhs_target->continuation.has_value() ||
      short_circuit_target == nullptr || short_circuit_target->continuation.has_value()) {
    return false;
  }

  const auto* rhs_block =
      find_bir_block_by_prepared_label(function,
                                       module.module.names,
                                       module.names,
                                       rhs_target->block_label);
  const auto rhs_label = rhs_block != nullptr
                             ? bir_block_label_id(module.module.names, module.names, *rhs_block)
                             : std::nullopt;
  const auto* rhs_condition =
      rhs_label.has_value()
          ? c4c::backend::prepare::find_prepared_branch_condition(*control_flow, *rhs_label)
          : nullptr;
  if (rhs_block == nullptr || rhs_condition == nullptr) {
    return false;
  }
  if (rhs_condition->true_label != rhs_target->continuation->true_label ||
      rhs_condition->false_label != rhs_target->continuation->false_label) {
    throw_prepared_control_flow_handoff_error(
        "prepared branch condition targets drifted from prepared block targets");
  }
  const auto rhs_compare =
      find_prepared_local_slot_compare_load(module,
                                            *addressing,
                                            *function_locations,
                                            function,
                                            *rhs_block,
                                            *rhs_label,
                                            *rhs_condition);
  if (!rhs_compare.has_value() ||
      rhs_compare->load->result.type != entry_compare->load->result.type ||
      rhs_compare->frame_slot_id != entry_compare->frame_slot_id ||
      rhs_compare->memory_operand != entry_compare->memory_operand) {
    return false;
  }

  const auto entry_immediate = prepared_branch_condition_immediate_operand(*entry_condition);
  const auto rhs_immediate = prepared_branch_condition_immediate_operand(*rhs_condition);
  if (!entry_immediate.has_value() || !rhs_immediate.has_value() ||
      *entry_immediate != *rhs_immediate ||
      !entry_condition->predicate.has_value() || !rhs_condition->predicate.has_value() ||
      !entry_condition->compare_type.has_value() || !rhs_condition->compare_type.has_value() ||
      *entry_condition->predicate != *rhs_condition->predicate ||
      *entry_condition->compare_type != *rhs_condition->compare_type ||
      (*entry_condition->predicate != c4c::backend::bir::BinaryOpcode::Ne &&
       *entry_condition->predicate != c4c::backend::bir::BinaryOpcode::Eq)) {
    return false;
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  const auto frame_adjust_bytes =
      normalize_x86_local_frame_adjust(addressing->frame_size_bytes != 0
                                           ? addressing->frame_size_bytes
                                           : module.stack_layout.frame_size_bytes);
  if (frame_adjust_bytes == 0) {
    throw_prepared_value_location_handoff_error(
        "defined function '" + function.name +
        "' local-slot short-circuit has no prepared frame size");
  }

  c4c::backend::x86::core::Text function_out;
  function_out.append_line(".globl " + symbol_name);
  function_out.append_line(".type " + symbol_name + ", @function");
  function_out.append_line(symbol_name + ":");
  function_out.append_line("    sub rsp, " + std::to_string(frame_adjust_bytes));

  const auto entry_type = entry_compare->load->result.type;
  for (std::size_t index = 0; index < entry_compare->instruction_index; ++index) {
    const auto* store =
        std::get_if<c4c::backend::bir::StoreLocalInst>(&entry_block->insts[index]);
    if (store == nullptr || !store->address.has_value() ||
        store->address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot ||
        store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        store->value.type != entry_type) {
      return false;
    }
    const auto store_memory =
        render_prepared_frame_slot_memory_operand(module, *addressing, entry_label, index, entry_type);
    if (!store_memory.has_value() || *store_memory != entry_compare->memory_operand) {
      throw_prepared_value_location_handoff_error(
          "defined function '" + function.name +
          "' local-slot short-circuit store drifted from prepared frame-slot access");
    }
    function_out.append_line("    mov " + *store_memory + ", " +
                             std::to_string(store->value.immediate));
  }

  const auto true_opcode =
      *entry_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne ? "jne" : "je";
  const auto false_opcode =
      *entry_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne ? "je" : "jne";
  const auto rhs_jump_opcode =
      rhs_target == &branch_plan->on_compare_true ? true_opcode : false_opcode;
  const auto rhs_label_text =
      c4c::backend::prepare::prepared_block_label(module.names, rhs_target->block_label);
  const auto render_load_compare =
      [&](const std::string& memory, c4c::backend::bir::TypeKind type) {
        if (type == c4c::backend::bir::TypeKind::I8) {
          function_out.append_line("    movsx eax, " + memory);
          function_out.append_line("    cmp al, " + std::to_string(*entry_immediate));
        } else {
          function_out.append_line("    mov eax, " + memory);
          function_out.append_line("    cmp eax, " + std::to_string(*entry_immediate));
        }
      };

  render_load_compare(entry_compare->memory_operand, entry_type);
  function_out.append_line("    " + std::string(rhs_jump_opcode) + " " +
                           render_function_local_label(function.name, rhs_label_text));
  if (!append_prepared_short_circuit_return_leaf(function_out,
                                                module,
                                                *function_locations,
                                                function,
                                                short_circuit_target->block_label,
                                                frame_adjust_bytes)) {
    return false;
  }

  const auto false_label_text = c4c::backend::prepare::prepared_block_label(
      module.names, rhs_target->continuation->false_label);
  function_out.append_line(render_function_local_label(function.name, rhs_label_text) + ":");
  render_load_compare(rhs_compare->memory_operand, rhs_compare->load->result.type);
  function_out.append_line("    " + std::string(false_opcode) + " " +
                           render_function_local_label(function.name, false_label_text));
  if (!append_prepared_short_circuit_return_leaf(function_out,
                                                module,
                                                *function_locations,
                                                function,
                                                rhs_target->continuation->true_label,
                                                frame_adjust_bytes)) {
    return false;
  }
  function_out.append_line(render_function_local_label(function.name, false_label_text) + ":");
  if (!append_prepared_short_circuit_return_leaf(function_out,
                                                module,
                                                *function_locations,
                                                function,
                                                rhs_target->continuation->false_label,
                                                frame_adjust_bytes)) {
    return false;
  }

  out.append_raw(function_out.take_text());
  return true;
}

bool append_supported_scalar_function(c4c::backend::x86::core::Text& out,
                                      const c4c::backend::prepare::PreparedBirModule& module,
                                      const c4c::backend::bir::Function& function,
                                      const Data& data) {
  if (append_prepared_loop_join_countdown_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_local_slot_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_local_slot_immediate_guard_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_local_slot_short_circuit_or_guard_function(out, module, function, data)) {
    return true;
  }
  validate_prepared_control_flow_handoff(module, function);
  if (append_prepared_i32_immediate_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_rematerialized_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_passthrough_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_binary_immediate_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_same_module_global_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_same_module_global_sub_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_direct_extern_call_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_param_zero_compare_join_return_function(out, module, function, data)) {
    return true;
  }
  if (append_prepared_i32_param_zero_branch_return_function(out, module, function, data)) {
    return true;
  }
  const auto folded_return = fold_supported_pointer_compare_return(module.module, function);
  if (!folded_return.has_value()) {
    return false;
  }

  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  out.append_line("    mov eax, " + std::to_string(*folded_return));
  out.append_line("    ret");
  return true;
}

void append_function_stub(c4c::backend::x86::core::Text& out,
                          const c4c::backend::prepare::PreparedBirModule& module,
                          const c4c::backend::bir::Function& function,
                          const Data& data) {
  validate_prepared_control_flow_handoff(module, function);
  const auto symbol_name = data.render_asm_symbol_name(function.name);
  out.append_line(".globl " + symbol_name);
  out.append_line(".type " + symbol_name + ", @function");
  out.append_line(symbol_name + ":");
  if (const auto function_name =
          c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name);
      function_name.has_value()) {
    append_grouped_authority_comments(out, module, *function_name);
  }
  out.append_line("    # x86 backend contract-first stub");
  out.append_line("    # behavior recovery will replace this body");
  if (function.return_type != c4c::backend::bir::TypeKind::Void) {
    out.append_line("    xor eax, eax");
  }
  out.append_line("    ret");
}

}  // namespace

std::string emit(const c4c::backend::prepare::PreparedBirModule& module) {
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  if (!c4c::backend::x86::abi::is_x86_target(target_profile)) {
    throw std::invalid_argument("x86::module::emit requires an x86 target profile");
  }

  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto data = make_data(module, target_triple);

  c4c::backend::x86::core::Text out;
  out.append_line(".intel_syntax noprefix");
  out.append_line(".text");

  bool emitted_any_function = false;
  bool emitted_only_supported_scalar_functions = true;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    emitted_any_function = true;
    if (!append_supported_scalar_function(out, module, function, data)) {
      emitted_only_supported_scalar_functions = false;
      out.append_line("# x86 backend contract-first module emitter");
      append_function_stub(out, module, function, data);
    }
  }

  if (!emitted_any_function) {
    out.append_line("# no defined functions");
  }

  if (!(emitted_any_function && emitted_only_supported_scalar_functions) ||
      !module.module.globals.empty()) {
    out.append_raw(data.emit_data());
  }
  return out.take_text();
}

}  // namespace c4c::backend::x86::module
