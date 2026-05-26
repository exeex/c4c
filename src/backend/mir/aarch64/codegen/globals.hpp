#pragma once

#include "alu.hpp"
#include "instruction.hpp"
#include "../module/module.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct BlockAddressMaterializationIndex {
  std::vector<const prepare::PreparedAddressMaterialization*> materializations;
  std::unordered_map<std::size_t,
                     std::vector<const prepare::PreparedAddressMaterialization*>>
      materializations_by_instruction;
};

[[nodiscard]] BlockAddressMaterializationIndex make_block_address_materialization_index(
    const module::BlockLoweringContext& context);
[[nodiscard]] bool emit_prepared_global_symbol_load_to_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);
[[nodiscard]] std::optional<module::MachineInstruction>
make_load_global_got_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global,
    BlockScalarLoweringState& scalar_state);
[[nodiscard]] PreparedAddressMaterializationRecordResult
make_prepared_address_materialization_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index);
[[nodiscard]] PreparedAddressMaterializationInstructionRecordResult
make_prepared_address_materialization_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index);
[[nodiscard]] std::optional<module::MachineInstruction> lower_address_materialization(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_address_materializations(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_address_materializations(
    const module::BlockLoweringContext& context,
    const BlockAddressMaterializationIndex& address_materializations,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] mir::TargetInstructionPrintResult print_address_materialization_instruction(
    const InstructionRecord& instruction,
    const AddressMaterializationRecord& address);

}  // namespace c4c::backend::aarch64::codegen
