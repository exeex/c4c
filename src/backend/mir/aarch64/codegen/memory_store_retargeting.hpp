#pragma once

#include "alu.hpp"
#include "instruction.hpp"
#include "../module/module.hpp"

#include <optional>

namespace c4c::backend::prepare {
struct PreparedAddressMaterializationLookups;
struct PreparedStackLayout;
struct PreparedValueHomeLookups;
}  // namespace c4c::backend::prepare

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;

[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store);

[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool resolve_frame_slot_memory_offset(
    const prepare::PreparedStackLayout& stack_layout,
    MemoryOperand& address);

[[nodiscard]] bool apply_stack_layout_to_memory_record(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedAddressMaterializationLookups* address_materialization_lookups,
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const bir::StoreLocalInst* local_store,
    MemoryInstructionRecord& record);

void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address);

void retarget_store_address_to_materialized_pointer(
    const bir::StoreLocalInst& store,
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address);

void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction);

void retarget_store_local_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction);

}  // namespace c4c::backend::aarch64::codegen
