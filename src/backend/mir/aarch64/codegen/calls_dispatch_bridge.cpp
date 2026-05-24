#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "calls_dispatch_bridge.hpp"
#include "calls.hpp"
#include "comparison_branch_fusion.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "memory_dynamic_stack.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

struct LocalAggregateAddressPublication {
  module::MachineInstruction instruction;
  RegisterOperand result_register;
};

[[nodiscard]] std::optional<LocalAggregateAddressPublication>
materialize_local_aggregate_address_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, value);
  if (!result_register.has_value() ||
      !abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }
  const auto offset =
      local_aggregate_address_frame_offset(context, result_register->value_name);
  if (!offset.has_value()) {
    return std::nullopt;
  }
  const auto address_register = abi::x_register(result_register->reg.index);
  result_register->reg = address_register;
  result_register->expected_view = abi::RegisterView::X;
  std::vector<std::string> lines;
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  std::string line = "add " + std::string{abi::register_name(address_register)} +
                     ", " + std::string{base} + ", #" + std::to_string(*offset);
  lines.push_back(std::move(line));
  auto instruction =
      make_select_chain_materialization_instruction(
          context, before_instruction_index, std::move(lines));
  if (!instruction.has_value()) {
    return std::nullopt;
  }
  return LocalAggregateAddressPublication{.instruction = std::move(*instruction),
                                          .result_register = *result_register};
}

[[nodiscard]] bool call_argument_allows_local_aggregate_address_publication(
    const bir::CallInst& call,
    std::size_t argument_index) {
  if (argument_index >= call.args.size()) {
    return false;
  }
  if (call.callee.rfind("llvm.", 0) == 0) {
    return false;
  }
  if (argument_index < call.arg_abi.size() &&
      call.arg_abi[argument_index].byval_copy) {
    return false;
  }
  return (argument_index < call.arg_types.size() &&
          call.arg_types[argument_index] == bir::TypeKind::Ptr) ||
         call.args[argument_index].type == bir::TypeKind::Ptr;
}

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    bool allow_local_aggregate_address_publication,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  if (allow_local_aggregate_address_publication) {
    if (auto local_address =
            materialize_local_aggregate_address_call_argument(
                context, value, before_instruction_index)) {
      record_emitted_scalar_register(scalar_state,
                                     local_address->result_register.value_name,
                                     local_address->result_register);
      lowered.push_back(std::move(local_address->instruction));
      return true;
    }
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  const auto producer_index =
      find_same_block_scalar_producer(context, value.name, before_instruction_index);
  if (!producer_index.has_value() || context.bir_block == nullptr) {
    if (has_same_block_load_local_producer(context, value, before_instruction_index)) {
      return true;
    }
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(&context.bir_block->insts[*producer_index]);
  if (binary == nullptr ||
      !is_scalar_call_argument_producer_opcode(binary->opcode)) {
    return true;
  }

  active_values.push_back(value.name);
  const bool lhs_ready =
      materialize_scalar_call_argument_value(context,
	                                             binary->lhs,
	                                             *producer_index,
	                                             allow_local_aggregate_address_publication,
	                                             scalar_state,
	                                             diagnostics,
	                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
	                                             binary->rhs,
	                                             *producer_index,
	                                             allow_local_aggregate_address_publication,
	                                             scalar_state,
	                                             diagnostics,
	                                             lowered,
                                             active_values);
  active_values.pop_back();
  if (!lhs_ready || !rhs_ready) {
    return false;
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  if (auto instruction = lower_scalar_instruction(context,
                                                  context.bir_block->insts[*producer_index],
                                                  *producer_index,
                                                  scalar_state,
                                                  diagnostics)) {
    const auto expected_result =
        make_named_prepared_result_register(context, binary->result);
    if (expected_result.has_value()) {
      if (auto* scalar =
              std::get_if<ScalarInstructionRecord>(&instruction->target.payload)) {
        scalar->result_register = *expected_result;
        if (scalar->scalar_alu.has_value()) {
          scalar->scalar_alu->result_register = *expected_result;
          if (const auto lhs_name = prepared_named_value_id(context, binary->lhs);
              lhs_name.has_value()) {
            if (const auto lhs_register =
                find_emitted_scalar_register(scalar_state, *lhs_name);
                lhs_register.has_value()) {
              auto lhs_operand = make_register_operand(*lhs_register);
              scalar->scalar_alu->lhs = lhs_operand;
              if (!scalar->inputs.empty()) {
                scalar->inputs[0] = std::move(lhs_operand);
              }
            }
          }
          if (const auto rhs_name = prepared_named_value_id(context, binary->rhs);
              rhs_name.has_value()) {
            if (const auto rhs_register =
                    find_emitted_scalar_register(scalar_state, *rhs_name);
                rhs_register.has_value()) {
              auto rhs_operand = make_register_operand(*rhs_register);
              scalar->scalar_alu->rhs = rhs_operand;
              if (scalar->inputs.size() > 1) {
                scalar->inputs[1] = std::move(rhs_operand);
              }
            }
          }
        }
        record_emitted_scalar_register(scalar_state,
                                       expected_result->value_name,
                                       *expected_result);
      }
    }
    lowered.push_back(std::move(*instruction));
    return true;
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  for (std::size_t argument_index = 0; argument_index < call.args.size(); ++argument_index) {
    const auto& argument = call.args[argument_index];
    if (auto select_chain =
            materialize_direct_global_select_chain_call_argument(context,
                                                                 argument,
                                                                 instruction_index,
                                                                 scalar_state)) {
      lowered.push_back(std::move(*select_chain));
      continue;
    }
    std::vector<std::string_view> active_values;
    const bool allow_local_aggregate_address_publication =
        call_argument_allows_local_aggregate_address_publication(call, argument_index);
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                allow_local_aggregate_address_publication,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}











[[nodiscard]] std::optional<bir::Value> find_bir_value_for_prepared_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto result = instruction_result_value(context.bir_block->insts[index - 1]);
    if (!result.has_value()) {
      continue;
    }
    const auto prepared_name = prepared_named_value_id(context, *result);
    if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
      return result;
    }
  }
  if (before_instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  if (const auto* call =
          std::get_if<bir::CallInst>(&context.bir_block->insts[before_instruction_index]);
      call != nullptr) {
    for (const auto& argument : call->args) {
      const auto prepared_name = prepared_named_value_id(context, argument);
      if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
        return argument;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_call_boundary_source_to_destination(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      !move_record->source_memory.has_value() ||
      move_record->source_memory_materializes_address ||
      !move_record->destination_register.has_value() ||
      !move_record->source_memory->result_value_name.has_value() ||
      move_record->source_memory->base_kind != MemoryBaseKind::FrameSlot ||
      find_emitted_scalar_register(
          scalar_state, *move_record->source_memory->result_value_name).has_value() ||
      !abi::is_gp_register(move_record->destination_register->reg)) {
    return std::nullopt;
  }

  const auto source_value =
      find_bir_value_for_prepared_name(
          context, *move_record->source_memory->result_value_name, instruction_index);
  if (!source_value.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const std::uint8_t target_index = move_record->destination_register->reg.index;
  const std::uint8_t scratch_index =
      scratches.front().index == target_index && scratches.size() > 1
          ? scratches[1].index
          : scratches.front().index;
  if (scratch_index == target_index) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *source_value,
                                          instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted = *move_record->destination_register;
  emitted.value_id = move_record->source_memory->result_value_id;
  emitted.value_name = *move_record->source_memory->result_value_name;
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

[[nodiscard]] c4c::SlotNameId local_load_effective_slot_id(
    const bir::LoadLocalInst& load) {
  if (load.address.has_value() &&
      load.address->base_slot_id != c4c::kInvalidSlotName) {
    return load.address->base_slot_id;
  }
  return load.slot_id;
}

[[nodiscard]] c4c::SlotNameId local_store_effective_slot_id(
    const bir::StoreLocalInst& store) {
  if (store.address.has_value() &&
      store.address->base_slot_id != c4c::kInvalidSlotName) {
    return store.address->base_slot_id;
  }
  return store.slot_id;
}

[[nodiscard]] std::int64_t local_load_effective_byte_offset(
    const bir::LoadLocalInst& load) {
  return load.address.has_value()
             ? load.address->byte_offset
             : static_cast<std::int64_t>(load.byte_offset);
}

[[nodiscard]] std::int64_t local_store_effective_byte_offset(
    const bir::StoreLocalInst& store) {
  return store.address.has_value()
             ? store.address->byte_offset
             : static_cast<std::int64_t>(store.byte_offset);
}

[[nodiscard]] bool same_local_scalar_slot_access(
    const bir::LoadLocalInst& load,
    const bir::StoreLocalInst& store) {
  const auto load_slot_id = local_load_effective_slot_id(load);
  const auto store_slot_id = local_store_effective_slot_id(store);
  if (load_slot_id != c4c::kInvalidSlotName ||
      store_slot_id != c4c::kInvalidSlotName) {
    if (load_slot_id == c4c::kInvalidSlotName ||
        store_slot_id == c4c::kInvalidSlotName ||
        load_slot_id != store_slot_id) {
      return false;
    }
  } else if (!load.slot_name.empty() || !store.slot_name.empty()) {
    if (load.slot_name.empty() || store.slot_name.empty() ||
        load.slot_name != store.slot_name) {
      return false;
    }
  }

  const auto load_size = dispatch_publication_scalar_type_size_bytes(load.result.type);
  const auto store_size = dispatch_publication_scalar_type_size_bytes(store.value.type);
  return load_size.has_value() &&
         store_size.has_value() &&
         *load_size == *store_size &&
         local_load_effective_byte_offset(load) ==
             local_store_effective_byte_offset(store);
}

struct LocalLoadStoredValue {
  bir::Value value;
  std::size_t store_instruction_index = 0;
};

[[nodiscard]] std::optional<LocalLoadStoredValue>
find_latest_same_block_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const std::size_t limit =
      std::min(load_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto store_index = index - 1;
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[store_index]);
    if (store != nullptr && same_local_scalar_slot_access(load, *store)) {
      return LocalLoadStoredValue{
          .value = store->value,
          .store_instruction_index = store_index,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<LocalLoadStoredValue>
resolve_indirect_callee_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::Value& callee_value,
    std::size_t call_instruction_index) {
  if (callee_value.kind != bir::Value::Kind::Named || callee_value.name.empty()) {
    return std::nullopt;
  }
  const auto* producer =
      find_same_block_named_producer(context, callee_value.name, call_instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer)
                                         : nullptr;
  const auto load_index = producer_instruction_index(context, producer);
  if (load == nullptr || !load_index.has_value()) {
    return std::nullopt;
  }
  auto stored =
      find_latest_same_block_local_load_store(context, *load, *load_index);
  if (!stored.has_value()) {
    return std::nullopt;
  }
  return stored;
}

[[nodiscard]] std::vector<std::uint8_t> indirect_callee_materialization_scratch_indices() {
  std::vector<std::uint8_t> indices;
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    indices.push_back(scratch.index);
  }
  for (const auto scratch : abi::indirect_call_scratch_registers()) {
    indices.push_back(scratch.index);
  }
  return indices;
}

[[nodiscard]] bool scratch_index_is_available(
    std::uint8_t candidate,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  return candidate != target_index &&
         std::find(occupied_indices.begin(), occupied_indices.end(), candidate) ==
             occupied_indices.end();
}

[[nodiscard]] std::optional<std::uint8_t> choose_scratch_index(
    const std::vector<std::uint8_t>& scratch_indices,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  for (const auto candidate : scratch_indices) {
    if (scratch_index_is_available(candidate, target_index, occupied_indices)) {
      return candidate;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool emit_indirect_callee_value_to_register_with_csel(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& scratch_indices,
    std::vector<std::uint8_t> occupied_indices,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 16U) {
    return false;
  }
  const auto* producer =
      value.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(context, value.name, before_instruction_index)
          : nullptr;
  const auto* select =
      producer != nullptr ? std::get_if<bir::SelectInst>(producer) : nullptr;
  if (select == nullptr) {
    const auto scratch_index =
        choose_scratch_index(scratch_indices, target_index, occupied_indices);
    return scratch_index.has_value() &&
           emit_value_publication_to_register(context,
                                              value,
                                              before_instruction_index,
                                              target_index,
                                              *scratch_index,
                                              lines,
                                              true);
  }

  const auto condition = branch_condition_suffix(select->predicate);
  const auto compare_view = scalar_view_for_type(select->compare_type);
  const auto result_view = scalar_view_for_type(value.type);
  if (!condition.has_value() || !compare_view.has_value() ||
      !result_view.has_value()) {
    return false;
  }

  const auto producer_index =
      producer_instruction_index(context, producer).value_or(before_instruction_index);
  const auto true_index =
      choose_scratch_index(scratch_indices, target_index, occupied_indices);
  if (!true_index.has_value()) {
    return false;
  }

  auto false_value = select->false_value;
  false_value.type = value.type;
  auto true_value = select->true_value;
  true_value.type = value.type;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        false_value,
                                                        producer_index,
                                                        target_index,
                                                        scratch_indices,
                                                        occupied_indices,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto true_occupied = occupied_indices;
  true_occupied.push_back(target_index);
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        true_value,
                                                        producer_index,
                                                        *true_index,
                                                        scratch_indices,
                                                        true_occupied,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto compare_occupied = occupied_indices;
  compare_occupied.push_back(target_index);
  compare_occupied.push_back(*true_index);
  const auto lhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!lhs_index.has_value()) {
    return false;
  }
  compare_occupied.push_back(*lhs_index);
  const auto rhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!rhs_index.has_value()) {
    return false;
  }

  auto lhs = select->lhs;
  lhs.type = select->compare_type;
  if (!emit_value_publication_to_register(context,
                                          lhs,
                                          producer_index,
                                          *lhs_index,
                                          *rhs_index,
                                          lines,
                                          true)) {
    return false;
  }
  std::string rhs_name;
  if (select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(select->rhs.immediate);
  } else {
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            producer_index,
                                            *rhs_index,
                                            *lhs_index,
                                            lines,
                                            true)) {
      return false;
    }
    const auto rhs_register = abi::gp_register(*rhs_index, *compare_view);
    if (!rhs_register.has_value()) {
      return false;
    }
    rhs_name = abi::register_name(*rhs_register);
  }

  const auto lhs_register = abi::gp_register(*lhs_index, *compare_view);
  const auto target_register = abi::gp_register(target_index, *result_view);
  const auto true_register = abi::gp_register(*true_index, *result_view);
  if (!lhs_register.has_value() || !target_register.has_value() ||
      !true_register.has_value()) {
    return false;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_register)} + ", " +
                  rhs_name);
  lines.push_back("csel " + std::string{abi::register_name(*target_register)} + ", " +
                  std::string{abi::register_name(*true_register)} + ", " +
                  std::string{abi::register_name(*target_register)} + ", " +
                  std::string{*condition});
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_indirect_call_callee_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (!call.is_indirect || !call.callee_value.has_value() ||
      !call_plan.is_indirect || !call_plan.indirect_callee.has_value()) {
    return std::nullopt;
  }
  const auto& callee = *call_plan.indirect_callee;
  if (callee.value_name == c4c::kInvalidValueName ||
      callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      callee.bank != prepare::PreparedRegisterBank::Gpr ||
      !callee.register_name.has_value()) {
    return std::nullopt;
  }

  bir::Value source_value = *call.callee_value;
  std::size_t source_before_index = instruction_index;
  if (const auto stored = resolve_indirect_callee_local_load_store(
          context, *call.callee_value, instruction_index);
      stored.has_value() &&
      select_chain_contains_direct_global_load(
          context, stored->value, stored->store_instruction_index)) {
    source_value = stored->value;
    source_before_index = stored->store_instruction_index;
  }

  const auto target_register = abi::parse_aarch64_register_name(*callee.register_name);
  if (!target_register.has_value() ||
      target_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto result_register = abi::gp_register(target_register->index, abi::RegisterView::X);
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = indirect_callee_materialization_scratch_indices();
  std::vector<std::string> lines;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_value,
                                                        source_before_index,
                                                        result_register->index,
                                                        scratches,
                                                        {},
                                                        lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*result_register},
      .occupied_registers = {abi::register_name(*result_register)},
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

void record_call_result_source_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.value_locations != nullptr) {
    const auto* bundle = prepare::find_prepared_move_bundle(
        *context.function.value_locations,
        prepare::PreparedMovePhase::AfterCall,
        call_plan.block_index,
        call_plan.instruction_index);
    if (bundle != nullptr) {
      for (const auto& move : bundle->moves) {
        if (move.destination_kind !=
                prepare::PreparedMoveDestinationKind::CallResultAbi ||
            move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
            move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
          continue;
        }
        const auto* home =
            find_value_home(context,
move.to_value_id);
        if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
          continue;
        }
        const auto expected_view =
            move.destination_register_name.has_value()
                ? abi::parse_aarch64_register_name(*move.destination_register_name)
                : std::nullopt;
        if (!expected_view.has_value() ||
            expected_view->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        abi::PreparedRegisterConversionResult converted;
        if (move.destination_register_placement.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_placement,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else if (move.destination_register_name.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_name,
              prepare::PreparedRegisterBank::Fpr,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else {
          continue;
        }
        if (!converted.reg.has_value() ||
            converted.reg->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        record_emitted_scalar_register(
            scalar_state,
            home->value_name,
            RegisterOperand{
                .reg = *converted.reg,
                .role = RegisterOperandRole::CallAbi,
                .value_id = home->value_id,
                .value_name = home->value_name,
                .prepared_class = prepare::PreparedRegisterClass::Float,
                .prepared_bank = prepare::PreparedRegisterBank::Fpr,
                .expected_view = converted.reg->view,
                .contiguous_width = move.destination_contiguous_width,
                .occupied_register_references = {*converted.reg},
                .occupied_registers = {abi::register_name(*converted.reg)},
            });
      }
    }
  }

  if (!call_plan.result.has_value() ||
      !call_plan.result->destination_value_id.has_value() ||
      !call_plan.result->source_register_name.has_value() ||
      call_plan.result->source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      context.function.value_locations == nullptr) {
    return;
  }
  const auto* home =
      find_value_home(context,
*call_plan.result->destination_value_id);
  if (home == nullptr) {
    return;
  }
  const auto parsed = abi::parse_aarch64_register_name(
      *call_plan.result->source_register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return;
  }
  auto viewed = *parsed;
  if (call_plan.result->source_register_placement.has_value()) {
    if (const auto converted =
            abi::convert_prepared_register(*call_plan.result->source_register_placement,
                                           prepare::PreparedRegisterClass::General,
                                           parsed->view);
        converted.reg.has_value()) {
      viewed = *converted.reg;
    }
  }
  record_emitted_scalar_register(
      scalar_state,
      home->value_name,
      RegisterOperand{
          .reg = viewed,
          .role = RegisterOperandRole::CallAbi,
          .value_id = home->value_id,
          .value_name = home->value_name,
          .prepared_class = prepare::PreparedRegisterClass::General,
          .prepared_bank = prepare::PreparedRegisterBank::Gpr,
          .expected_view = viewed.view,
          .contiguous_width = call_plan.result->source_contiguous_width,
          .occupied_register_references = {viewed},
          .occupied_registers = {abi::register_name(viewed)},
      });
}

[[nodiscard]] bool move_source_aliases_destination(
    const module::MachineInstruction& source_instruction,
    const module::MachineInstruction& destination_instruction) {
  const auto* source =
      std::get_if<CallBoundaryMoveInstructionRecord>(&source_instruction.target.payload);
  const auto* destination =
      std::get_if<CallBoundaryMoveInstructionRecord>(&destination_instruction.target.payload);
  return source != nullptr &&
         destination != nullptr &&
         destination->move.reason != "callee_saved_preservation_home_population" &&
         source->source_register.has_value() &&
         destination->destination_register.has_value() &&
         registers_alias(*source->source_register, *destination->destination_register);
}

[[nodiscard]] bool call_boundary_move_reloads_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  const auto* move =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move == nullptr ||
      move->phase != prepare::PreparedMovePhase::BeforeCall ||
      move->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move->move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !move->source_memory.has_value() ||
      !move->destination_register.has_value() ||
      (!move->source_memory->result_value_id.has_value() &&
       !move->source_memory->result_value_name.has_value())) {
    return false;
  }
  for (const auto& materialized : materialized_addresses) {
    const auto* address =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address == nullptr || !address->result_register.has_value() ||
        !registers_alias(*address->result_register, *move->destination_register)) {
      continue;
    }
    const bool same_value_id =
        move->source_memory->result_value_id.has_value() &&
        address->result_value_id == move->source_memory->result_value_id;
    const bool same_value_name =
        move->source_memory->result_value_name.has_value() &&
        address->result_value_name == *move->source_memory->result_value_name;
    if (same_value_id || same_value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
order_before_call_moves_for_source_preservation(
    std::vector<module::MachineInstruction> moves) {
  std::vector<module::MachineInstruction> ordered;
  ordered.reserve(moves.size());
  while (!moves.empty()) {
    auto selected = moves.begin();
    for (auto candidate = moves.begin(); candidate != moves.end(); ++candidate) {
      const bool protects_live_source =
          std::any_of(moves.begin(), moves.end(), [&](const auto& other) {
            return &other != &*candidate &&
                   move_source_aliases_destination(*candidate, other);
          });
      if (protects_live_source) {
        selected = candidate;
        break;
      }
    }
    ordered.push_back(std::move(*selected));
    moves.erase(selected);
  }
  return ordered;
}

[[nodiscard]] std::vector<module::MachineInstruction>
materialize_missing_frame_slot_call_arguments(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
        !argument.source_value_id.has_value() ||
        argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        argument.destination_contiguous_width > 1) {
      continue;
    }
    const auto* home =
        find_value_home(context,
*argument.source_value_id);
    if (home == nullptr ||
        find_emitted_scalar_register(scalar_state, home->value_name).has_value()) {
      continue;
    }
    const auto source_value =
        find_bir_value_for_prepared_name(context, home->value_name, instruction_index);
    if (!source_value.has_value()) {
      continue;
    }
    const auto expected_view = scalar_view_for_type(source_value->type);
    if (!expected_view.has_value()) {
      continue;
    }
    const auto prepared_class = prepare::PreparedRegisterClass::General;
    abi::PreparedRegisterConversionResult converted;
    if (argument.destination_register_placement.has_value()) {
      converted =
          abi::convert_prepared_register(*argument.destination_register_placement,
                                         prepared_class,
                                         expected_view);
    } else if (argument.destination_register_name.has_value()) {
      converted =
          abi::convert_prepared_register(*argument.destination_register_name,
                                         argument.destination_register_bank,
                                         prepared_class,
                                         expected_view);
    } else {
      continue;
    }
    if (!converted.reg.has_value()) {
      continue;
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      continue;
    }
    const std::uint8_t target_index = converted.reg->index;
    const std::uint8_t scratch_index =
        scratches.front().index == target_index && scratches.size() > 1
            ? scratches[1].index
            : scratches.front().index;
    if (scratch_index == target_index) {
      continue;
    }
    std::vector<std::string> lines;
    if (!emit_value_publication_to_register(context,
                                            *source_value,
                                            instruction_index,
                                            target_index,
                                            scratch_index,
                                            lines) ||
        lines.empty()) {
      continue;
    }
    RegisterOperand emitted{
        .reg = *converted.reg,
        .role = RegisterOperandRole::CallAbi,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_class = prepared_class,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = expected_view,
        .contiguous_width = argument.destination_contiguous_width,
        .occupied_register_references = {*converted.reg},
        .occupied_registers = {abi::register_name(*converted.reg)},
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
    if (auto materialized =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*materialized));
    }
  }
  return lowered;
}

[[nodiscard]] std::vector<const prepare::PreparedCallPreservedValue*>
first_stack_preserved_values_for_call_fallback(
    const prepare::PreparedCallPlansFunction& call_plans,
    const prepare::PreparedCallPlan& current_call_plan) {
  std::vector<const prepare::PreparedCallPreservedValue*> values;
  std::vector<unsigned char> seen_stack_values;
  for (const auto& call : call_plans.calls) {
    const bool is_current = call.block_index == current_call_plan.block_index &&
                            call.instruction_index == current_call_plan.instruction_index;
    for (const auto& preserved : call.preserved_values) {
      if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
          preserved.value_name == c4c::kInvalidValueName ||
          !preserved.slot_id.has_value() ||
          !preserved.stack_offset_bytes.has_value() ||
          !preserved.stack_size_bytes.has_value() ||
          *preserved.stack_size_bytes == 0) {
        continue;
      }
      if (preserved.value_id >= seen_stack_values.size()) {
        seen_stack_values.resize(preserved.value_id + 1U, 0U);
      }
      if (seen_stack_values[preserved.value_id] != 0U) {
        continue;
      }
      seen_stack_values[preserved.value_id] = 1U;
      if (is_current) {
        values.push_back(&preserved);
      }
    }
    if (is_current) {
      break;
    }
  }
  return values;
}

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr &&
                     context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared,
                       context.function.control_flow->function_name)
                 : nullptr);
  const auto* first_stack_values =
      call_plans == nullptr || context.function.call_plan_lookups == nullptr
          ? nullptr
          : prepare::first_indexed_stack_preserved_values_for_call(
                *context.function.call_plan_lookups, *call_plans, call_plan);
  std::vector<const prepare::PreparedCallPreservedValue*> fallback_values;
  if (call_plans != nullptr && context.function.call_plan_lookups == nullptr) {
    fallback_values =
        first_stack_preserved_values_for_call_fallback(*call_plans, call_plan);
  } else if (call_plans == nullptr) {
    fallback_values.reserve(call_plan.preserved_values.size());
    for (const auto& preserved : call_plan.preserved_values) {
      fallback_values.push_back(&preserved);
    }
  }
  const auto& values =
      first_stack_values != nullptr ? *first_stack_values : fallback_values;
  for (const auto* preserved_ptr : values) {
    if (preserved_ptr == nullptr) {
      continue;
    }
    const auto& preserved = *preserved_ptr;
    if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
        preserved.value_name == c4c::kInvalidValueName ||
        !preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0) {
      continue;
    }
    const auto emitted =
        find_emitted_scalar_register(scalar_state, preserved.value_name);
    std::optional<abi::RegisterView> expected_view;
    if (*preserved.stack_size_bytes == 8) {
      expected_view = abi::RegisterView::X;
    } else if (*preserved.stack_size_bytes == 4 ||
               *preserved.stack_size_bytes == 2 ||
               *preserved.stack_size_bytes == 1) {
      expected_view = abi::RegisterView::W;
    } else {
      continue;
    }
    std::optional<abi::RegisterReference> source_register;
    if (emitted.has_value() && abi::is_gp_register(emitted->reg)) {
      source_register = emitted->reg;
    } else if (context.function.value_locations != nullptr) {
      const auto* home =
          find_value_home(context,
preserved.value_id);
      if (home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register &&
          home->register_name.has_value()) {
        const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
        if (parsed.has_value() &&
            parsed->bank == abi::RegisterBank::GeneralPurpose) {
          source_register = *parsed;
        }
      }
    }
    if (!source_register.has_value()) {
      continue;
    }
    const auto source_reg = abi::gp_register(source_register->index, *expected_view);
    if (!source_reg.has_value()) {
      continue;
    }

    std::vector<std::string> lines;
    lines.push_back("str " + std::string(abi::register_name(*source_reg)) + ", " +
                    frame_slot_address(context.function, *preserved.stack_offset_bytes));
    if (auto published =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*published));
    }
  }
  return lowered;
}

}  // namespace c4c::backend::aarch64::codegen
