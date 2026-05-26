#pragma once

#include "alu.hpp"
#include "../module/module.hpp"
#include "../../query.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store);

[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool emit_scalar_conversion_cast_to_register(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t cast_instruction_index,
    const RegisterOperand& target_register,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_local_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const BlockScalarLoweringState& scalar_state,
    const module::MachineBlock& block);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

[[nodiscard]] std::optional<std::string> prepared_global_symbol_from_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);

[[nodiscard]] bool emit_global_symbol_address_to_register(
    std::string_view symbol,
    std::int64_t delta,
    std::uint8_t target_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool emit_pointer_base_plus_offset_to_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& value_home,
    std::uint8_t target_index,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<std::size_t> store_local_frame_slot_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_pointer_base_plus_offset_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

[[nodiscard]] bool is_store_global_select_snapshot_run_instruction(const bir::Inst& inst);

void lower_pending_store_global_stack_value_publications(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::unordered_set<c4c::ValueNameId>& published_stack_values,
    module::MachineBlock& block);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    std::unordered_set<c4c::ValueNameId>* published_stack_values = nullptr,
    bool stack_homes_only = false);

}  // namespace c4c::backend::aarch64::codegen
