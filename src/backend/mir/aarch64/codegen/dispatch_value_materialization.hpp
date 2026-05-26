#pragma once

#include "alu.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] bool emit_fp_immediate_to_register(std::vector<std::string>& lines,
                                                 const bir::Value& value,
                                                 abi::RegisterReference fp_destination,
                                                 std::uint8_t gp_scratch_index);

[[nodiscard]] bool emit_fp_value_to_register(const module::BlockLoweringContext& context,
                                             const bir::Value& value,
                                             std::size_t before_instruction_index,
                                             abi::RegisterReference destination,
                                             std::uint8_t gp_scratch_index,
                                             std::vector<std::string>& lines);

[[nodiscard]] bool emit_prepared_global_symbol_load_to_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool emit_prepared_va_list_field_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::uint8_t target_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool emit_value_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    bool reload_current_memory_loads = false);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_local_slot_address_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
make_load_global_got_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_fp_binary_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_stack(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
