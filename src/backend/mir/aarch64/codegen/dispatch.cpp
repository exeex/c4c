#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "comparison_branch_fusion.hpp"
#include "dispatch_edge_copies.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_publication_common.hpp"
#include "dispatch_value_materialization.hpp"
#include "memory_store_sources.hpp"
#include "f128.hpp"
#include "fp_value_materialization.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "memory_dynamic_stack.hpp"
#include "operands.hpp"
#include "prepared_value_home_materialization.hpp"
#include "prologue.hpp"
#include "returns.hpp"
#include "variadic.hpp"
#include "../../../prealloc/target_register_profile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

constexpr std::size_t kStackPointerAlignmentBytes = 16;

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(bir::TerminatorKind kind) {
  switch (kind) {
    case bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool binary_uses_named_value(const bir::Inst& inst,
                                           std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr) {
    return false;
  }
  const auto matches = [&](const bir::Value& value) {
    return value.kind == bir::Value::Kind::Named && value.name == value_name;
  };
  return matches(binary->lhs) || matches(binary->rhs);
}

[[nodiscard]] bool instruction_result_has_stack_home(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  const auto result_value = instruction_result_value(inst);
  if (!result_value.has_value() ||
      result_value->kind != bir::Value::Kind::Named ||
      result_value->name.empty()) {
    return false;
  }
  const auto result_value_name = prepared_named_value_id(context, *result_value);
  if (!result_value_name.has_value()) {
    return false;
  }
  const auto* result_home = find_value_home(context, *result_value_name);
  return result_home != nullptr &&
         result_home->kind == prepare::PreparedValueHomeKind::StackSlot;
}

[[nodiscard]] bool is_store_local_instruction(const bir::Inst& inst) {
  return std::get_if<bir::StoreLocalInst>(&inst) != nullptr;
}

[[nodiscard]] bool before_return_publication_already_emitted(
    const BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      move_record->phase != prepare::PreparedMovePhase::BeforeReturn ||
      move_record->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      !move_record->destination_register.has_value()) {
    return false;
  }
  const auto emitted =
      find_emitted_scalar_register(scalar_state,
                                   move_record->destination_register->value_name);
  return emitted.has_value() &&
         registers_alias(*emitted, *move_record->destination_register);
}

void record_before_return_publication(BlockScalarLoweringState& scalar_state,
                                      const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      move_record->phase != prepare::PreparedMovePhase::BeforeReturn ||
      move_record->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      !move_record->destination_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 move_record->destination_register->value_name,
                                 *move_record->destination_register);
}

struct NarrowLocalStorePublication {
  bir::Value stored_value;
  std::size_t instruction_index = 0;
};



























[[nodiscard]] std::optional<module::BlockLoweringContext> block_context_for_label(
    const module::BlockLoweringContext& context,
    c4c::BlockLabelId label) {
  if (context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.function.control_flow->blocks.size(); ++index) {
    const auto& block = context.function.control_flow->blocks[index];
    if (block.block_label == label) {
      return module::BlockLoweringContext{
          .function = context.function,
          .control_flow_block = &block,
          .bir_block = find_bir_block(context.function, block),
          .block_index = index,
      };
    }
  }
  return std::nullopt;
}



struct EdgeProducerContext {
  module::BlockLoweringContext context;
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
};




































[[nodiscard]] bool lower_store_local_with_address_materialization(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr) {
    return false;
  }
  if (auto pointer_address_store =
          lower_pointer_base_plus_offset_store_local_publication(
              context, inst, instruction_index)) {
    block.instructions.push_back(std::move(*pointer_address_store));
    return true;
  }

  auto materialized = lower_address_materialization(context, instruction_index, diagnostics);
  if (!materialized.has_value()) {
    return false;
  }
  std::optional<RegisterOperand> materialized_address;
  if (const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized->target.payload);
      address_record != nullptr && address_record->result_register.has_value()) {
    materialized_address = *address_record->result_register;
  }

  record_address_materialization_result(scalar_state, *materialized);
  block.instructions.push_back(std::move(*materialized));

  const auto diagnostic_count = diagnostics.entries.size();
  auto lowered_memory =
      lower_memory_instruction(context, inst, instruction_index, diagnostics);
  if (lowered_memory.handled && lowered_memory.instruction.has_value()) {
    if (materialized_address.has_value()) {
      retarget_pointer_store_value_to_materialized_address(
          *lowered_memory.instruction, *materialized_address);
      retarget_store_address_to_materialized_pointer(
          *store, *lowered_memory.instruction, *materialized_address);
    }
    if (auto value_publication =
            lower_store_local_value_publication(context,
                                                inst,
                                                instruction_index,
                                                *lowered_memory.instruction,
                                                scalar_state,
                                                block)) {
      block.instructions.push_back(std::move(*value_publication));
    }
    record_memory_result(scalar_state, *lowered_memory.instruction);
    block.instructions.push_back(std::move(*lowered_memory.instruction));
  } else if (lowered_memory.handled) {
    if (auto pointer_address_store =
            lower_pointer_base_plus_offset_store_local_publication(
                context, inst, instruction_index)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*pointer_address_store));
      return lowered_memory.handled;
    }
    if (auto pointer_store =
            lower_stack_homed_pointer_store_writeback(context, inst, instruction_index)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*pointer_store));
      return lowered_memory.handled;
    }
    if (auto formal_store =
            lower_fixed_formal_store_local_publication(
                context, inst, instruction_index, scalar_state)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*formal_store));
    }
  }
  return lowered_memory.handled;
}

[[nodiscard]] bool lower_scalar_with_address_materialization(
    const module::BlockLoweringContext& context,
    const BlockAddressMaterializationIndex& address_materializations,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->result.kind != bir::Value::Kind::Named ||
      context.bir_block == nullptr) {
    return false;
  }

  auto materialized_addresses =
      lower_address_materializations(
          context, address_materializations, instruction_index, diagnostics);
  if (materialized_addresses.empty()) {
    return false;
  }

  auto trial_scalar_state = scalar_state;
  for (const auto& materialized : materialized_addresses) {
    record_address_materialization_result(trial_scalar_state, materialized);
  }
  std::optional<module::MachineInstruction> lowered_scalar;
  if (binary->result.type == bir::TypeKind::Ptr &&
      instruction_index + 1 < context.bir_block->insts.size()) {
    const auto* next_store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[instruction_index + 1]);
    if (next_store != nullptr &&
        next_store->value.kind == bir::Value::Kind::Named &&
        next_store->value.type == bir::TypeKind::Ptr &&
        next_store->value.name == binary->result.name) {
      lowered_scalar =
          lower_scalar_instruction(context, inst, instruction_index, trial_scalar_state, diagnostics);
    }
  } else if (bir::is_compare_opcode(binary->opcode)) {
    lowered_scalar = lower_scalar_control_value_instruction(
        context, inst, instruction_index, trial_scalar_state, diagnostics, true);
  }
  if (!lowered_scalar.has_value()) {
    return false;
  }

  scalar_state = std::move(trial_scalar_state);
  for (auto& materialized : materialized_addresses) {
    block.instructions.push_back(std::move(materialized));
  }
  block.instructions.push_back(std::move(*lowered_scalar));
  return true;
}

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}


[[nodiscard]] DispatchBranchFusionHooks make_dispatch_branch_fusion_hooks() {
  return DispatchBranchFusionHooks{
      .scalar_view_for_type = scalar_view_for_type,
      .emit_value_publication_to_register = emit_value_publication_to_register,
      .current_block_entry_publication_register =
          current_block_entry_publication_register,
      .find_same_block_named_producer = find_same_block_named_producer,
      .producer_instruction_index = producer_instruction_index,
      .prepared_value_home_for_value = prepared_value_home_for_value,
      .value_has_current_block_entry_publication =
          value_has_current_block_entry_publication,
      .emit_prepared_value_home_to_register =
          emit_prepared_value_home_to_register,
      .fixed_slots_use_frame_pointer = fixed_slots_use_frame_pointer,
  };
}



}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  record_current_block_entry_publication_registers(context, scalar_state);
  const auto branch_fusion_hooks = make_dispatch_branch_fusion_hooks();
  std::unordered_set<c4c::ValueNameId> published_store_global_stack_values;

  for (auto& entry_formal : lower_entry_formal_publications(context, scalar_state)) {
    block.instructions.push_back(std::move(entry_formal));
  }
  for (auto& block_entry_move : lower_value_moves(
           context,
           prepare::PreparedMovePhase::BlockEntry,
           0,
           diagnostics)) {
    if (!should_emit_block_entry_edge_copy_move(context, block_entry_move)) {
      continue;
    }
    record_call_boundary_destination(block_entry_move, scalar_state);
    block.instructions.push_back(std::move(block_entry_move));
  }
  if (context.bir_block != nullptr) {
    const auto join_parallel_copy_cache =
        build_current_block_join_parallel_copy_cache(context);
    std::optional<BlockAddressMaterializationIndex> address_materialization_index;
    auto current_address_materialization_index = [&]() -> const BlockAddressMaterializationIndex& {
      if (!address_materialization_index.has_value()) {
        address_materialization_index = make_block_address_materialization_index(context);
      }
      return *address_materialization_index;
    };
    std::size_t prepared_memory_instruction_index = 0;
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      const bool is_memory_inst =
          std::get_if<bir::LoadLocalInst>(&inst) != nullptr ||
          std::get_if<bir::LoadGlobalInst>(&inst) != nullptr ||
          std::get_if<bir::StoreLocalInst>(&inst) != nullptr ||
          std::get_if<bir::StoreGlobalInst>(&inst) != nullptr;
      const std::size_t memory_instruction_index =
          is_memory_inst ? ++prepared_memory_instruction_index : instruction_index;
      const auto* retained_index_access =
          is_memory_inst ? prepared_memory_access(context, instruction_index) : nullptr;
      const auto* prepared_index_access =
          is_memory_inst ? prepared_memory_access(context, memory_instruction_index) : nullptr;
      const bool use_prepared_memory_index =
          is_memory_inst &&
          memory_instruction_index != instruction_index &&
          !prepared_memory_access_matches_instruction(
              context, retained_index_access, inst) &&
          prepared_memory_access_matches_instruction(
              context, prepared_index_access, inst);
      const std::size_t memory_lowering_index =
          use_prepared_memory_index ? memory_instruction_index : instruction_index;
      const bool can_retry_prepared_memory_index =
          is_memory_inst &&
          memory_instruction_index != memory_lowering_index &&
          prepared_memory_access_matches_instruction(
              context, prepared_index_access, inst);
      if (std::get_if<bir::BinaryInst>(&inst) != nullptr) {
        const bool stack_home_fused_compare_branch =
            is_fused_compare_branch_support_instruction(
                context, inst, scalar_state, branch_fusion_hooks) &&
            (lower_stack_home_fused_compare_branch(context, branch_fusion_hooks)
                 .has_value() ||
             lower_constant_rhs_fused_compare_branch(context, branch_fusion_hooks)
                 .has_value());
        for (auto& before_instruction_move : lower_value_moves(
                 context,
                 prepare::PreparedMovePhase::BeforeInstruction,
                 instruction_index,
                 diagnostics)) {
          if (block_entry_move_clobbers_current_join_publication(
                  context, before_instruction_move)) {
            continue;
          }
          if (!call_boundary_move_reloads_prepared_stack_source(
                  before_instruction_move)) {
            continue;
          }
          if (stack_home_fused_compare_branch) {
            continue;
          }
          record_call_boundary_destination(before_instruction_move, scalar_state);
          record_call_boundary_source_in_destination(before_instruction_move,
                                                     scalar_state);
          block.instructions.push_back(std::move(before_instruction_move));
        }
      }
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        // Dispatch sequences call lowering; the calls owner API handles the
        // call-boundary mechanics invoked at each routing point.
        if (auto dynamic_stack = lower_dynamic_stack_helper_call(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*dynamic_stack));
          ++result.visited_operations;
          continue;
        }
        if (call->inline_asm.has_value() ||
            has_prepared_inline_asm_carrier(context, instruction_index)) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (has_prepared_intrinsic_carrier(context, instruction_index)) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          const bool inline_variadic_entry_helper =
              variadic_entry_helper_kind(call->callee).has_value();
          auto argument_producers =
              lower_scalar_call_argument_producers(context,
                                                   *call_plan,
                                                   call->args,
                                                   instruction_index,
                                                   scalar_state,
                                                   diagnostics);
          for (auto& argument_producer : argument_producers) {
            block.instructions.push_back(std::move(argument_producer));
          }
          for (auto& preserved_stack_publication :
               publish_stack_preserved_call_values(
                   context, *call_plan, instruction_index, scalar_state)) {
            block.instructions.push_back(std::move(preserved_stack_publication));
          }
          auto materialized_addresses =
              lower_address_materializations(
                  context,
                  current_address_materialization_index(),
                  instruction_index,
                  diagnostics);
          auto before_call_moves =
              inline_variadic_entry_helper
                  ? std::vector<module::MachineInstruction>{}
                  : lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          if (auto materialized_callee =
                  materialize_indirect_call_callee_to_prepared_register(
                      context, *call, *call_plan, instruction_index, scalar_state)) {
            block.instructions.push_back(std::move(*materialized_callee));
          }
          for (auto& before_call_move : before_call_moves) {
            retarget_call_boundary_source_to_emitted_scalar(
                before_call_move, scalar_state);
          }
          std::vector<module::MachineInstruction> deferred_before_call_moves;
          for (auto& before_call_move : before_call_moves) {
            if (source_register_conflicts_with_materialized_address(
                    before_call_move, materialized_addresses)) {
              record_call_boundary_destination(before_call_move, scalar_state);
              block.instructions.push_back(std::move(before_call_move));
            } else {
              deferred_before_call_moves.push_back(std::move(before_call_move));
            }
          }
          for (auto& materialized : materialized_addresses) {
            if (const auto* address_record =
                    std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
                address_record != nullptr && address_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             address_record->result_value_name,
                                             *address_record->result_register);
            }
            block.instructions.push_back(std::move(materialized));
          }
          for (auto& before_call_move :
               order_before_call_moves_for_source_preservation(
                   std::move(deferred_before_call_moves))) {
            if (call_boundary_move_reloads_materialized_address(
                    before_call_move, materialized_addresses)) {
              continue;
            }
            retarget_call_boundary_source_to_emitted_scalar(
                before_call_move, scalar_state);
            if (auto materialized =
                    materialize_call_boundary_source_to_destination(
                        context, before_call_move, instruction_index, scalar_state)) {
              block.instructions.push_back(std::move(*materialized));
            } else {
              record_call_boundary_destination(before_call_move, scalar_state);
              block.instructions.push_back(std::move(before_call_move));
            }
          }
          if (!inline_variadic_entry_helper) {
            for (auto& materialized_argument :
                 materialize_missing_frame_slot_call_arguments(
                     context, *call_plan, instruction_index, scalar_state)) {
              block.instructions.push_back(std::move(materialized_argument));
            }
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          clear_call_clobbered_emitted_scalar_registers(scalar_state);
          if (call_plan != nullptr) {
            record_call_result_source_register(context, *call_plan, scalar_state);
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              record_call_boundary_destination(after_call_move, scalar_state);
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (lower_store_local_with_address_materialization(
                     context,
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (cached_current_block_join_parallel_copy_incoming_expression(
                     join_parallel_copy_cache, context, instruction_index, inst) &&
                 !instruction_result_has_stack_home(context, inst)) {
        continue;
      } else if (lower_scalar_with_address_materialization(
                     context,
                     current_address_materialization_index(),
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        record_address_materialization_result(scalar_state, *lowered);
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_mul =
                     lower_scalar_mul_with_distinct_rhs_scratch(
                         context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered_mul));
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_copy =
                     lower_i128_copy_instruction(context, inst, instruction_index, diagnostics);
                 lowered_i128_copy.handled) {
        if (lowered_i128_copy.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_copy.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
                 load_global != nullptr) {
        if (auto got_load =
                make_load_global_got_materialization_instruction(
                    context, instruction_index, *load_global, scalar_state)) {
          block.instructions.push_back(std::move(*got_load));
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, memory_lowering_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (!lowered_ordinary_memory.instruction.has_value() &&
              can_retry_prepared_memory_index &&
              memory_instruction_index != memory_lowering_index) {
            lowered_ordinary_memory =
                lower_memory_instruction(context, inst, memory_instruction_index, diagnostics);
          }
          if (lowered_ordinary_memory.instruction.has_value()) {
            retarget_memory_result_to_prepared_home(
                context, *lowered_ordinary_memory.instruction);
            record_memory_result(scalar_state, *lowered_ordinary_memory.instruction);
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      } else if (auto lowered = lower_prepared_scalar_float_alu_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_fp_binary_publication_to_prepared_register(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_fused_compare_branch_support_instruction(
                     context, inst, scalar_state, branch_fusion_hooks)) {
        continue;
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_register(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_stack(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (cached_current_block_join_parallel_copy_source(
                     join_parallel_copy_cache, context, instruction_index, inst) &&
                 !instruction_result_has_stack_home(context, inst)) {
        continue;
      } else if (auto lowered =
                     lower_stack_homed_pointer_value_load_publication(
                         context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_f128_transport =
                     lower_f128_transport_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_transport.handled) {
        if (lowered_f128_transport.instruction.has_value()) {
          retarget_memory_result_to_prepared_home(
              context, *lowered_f128_transport.instruction);
          record_memory_result(scalar_state, *lowered_f128_transport.instruction);
          block.instructions.push_back(std::move(*lowered_f128_transport.instruction));
        }
      } else if (auto lowered = lower_local_slot_address_publication(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_control_value_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            retarget_memory_result_to_prepared_home(context, *lowered_memory.instruction);
            record_memory_result(scalar_state, *lowered_memory.instruction);
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else {
          if (std::get_if<bir::StoreGlobalInst>(&inst) != nullptr) {
            lower_pending_store_global_stack_value_publications(
                context,
                instruction_index,
                published_store_global_stack_values,
                block);
          }
          const auto diagnostic_count = diagnostics.entries.size();
          auto lowered_ordinary_memory =
              lower_memory_instruction(context, inst, memory_lowering_index, diagnostics);
          if (lowered_ordinary_memory.handled) {
            if (!lowered_ordinary_memory.instruction.has_value() &&
                can_retry_prepared_memory_index &&
                memory_instruction_index != memory_lowering_index) {
              lowered_ordinary_memory =
                  lower_memory_instruction(context, inst, memory_instruction_index, diagnostics);
            }
            if (lowered_ordinary_memory.instruction.has_value()) {
              retarget_pointer_store_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              retarget_store_local_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              retarget_fpr_call_result_store_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              if (auto value_publication =
                      lower_store_local_value_publication(
                          context,
                          inst,
                          instruction_index,
                          *lowered_ordinary_memory.instruction,
                          scalar_state,
                          block)) {
                block.instructions.push_back(std::move(*value_publication));
              }
              if (auto value_publication =
                      lower_store_global_value_publication(
                          context,
                          inst,
                          instruction_index,
                          *lowered_ordinary_memory.instruction,
                          &published_store_global_stack_values)) {
                block.instructions.push_back(std::move(*value_publication));
              }
              retarget_memory_result_to_prepared_home(
                  context, *lowered_ordinary_memory.instruction);
              record_memory_result(scalar_state, *lowered_ordinary_memory.instruction);
              block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
            } else if (auto pointer_address_store =
                           lower_pointer_base_plus_offset_store_local_publication(
                               context, inst, memory_lowering_index)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*pointer_address_store));
            } else if (auto pointer_store =
                           lower_stack_homed_pointer_store_writeback(
                               context, inst, memory_lowering_index)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*pointer_store));
            } else if (auto formal_store =
                           lower_fixed_formal_store_local_publication(
                               context, inst, memory_lowering_index, scalar_state)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*formal_store));
            }
          } else {
            append_unsupported_instruction_diagnostic(
                diagnostics, context, inst, instruction_index);
          }
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    const std::size_t return_instruction_index =
        context.bir_block != nullptr ? context.bir_block->insts.size() : 0;
    for (auto& before_return_move :
         lower_before_return_moves(context, return_instruction_index, diagnostics)) {
      if (before_return_publication_already_emitted(scalar_state,
                                                    before_return_move)) {
        continue;
      }
      record_before_return_publication(scalar_state, before_return_move);
      block.instructions.push_back(std::move(before_return_move));
    }
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    for (auto& edge_source :
         lower_predecessor_select_parallel_copy_sources(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(edge_source));
    }
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (fused_compare_uses_selected_operand(context, branch_fusion_hooks)) {
      for (auto& publication :
           lower_missing_fused_compare_operand_publications(
               context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(publication));
      }
    }
    if (auto lowered =
            lower_current_block_entry_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_stack_home_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
            lower_fused_compare_branch_from_emitted_cast(context, scalar_state, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_constant_rhs_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_materialized_compare_condition_branch(context, scalar_state, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else {
      for (auto& publication :
           lower_missing_fused_compare_operand_publications(
               context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(publication));
      }
      if (auto publication =
              lower_missing_conditional_branch_condition_publication(
                  context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*publication));
      }
      if (auto lowered = lower_conditional_branch_from_emitted_condition(
              context, scalar_state, branch_fusion_hooks)) {
        block.instructions.push_back(std::move(*lowered));
        block.successors = make_conditional_branch_successors(context);
      } else if (auto lowered =
              lower_prepared_conditional_branch_terminator(
                  context, diagnostics, &scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
        block.successors = make_conditional_branch_successors(context);
      }
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen
