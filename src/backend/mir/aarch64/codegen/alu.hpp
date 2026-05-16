#pragma once

#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace c4c::backend::aarch64::codegen {

struct BlockScalarLoweringState {
  std::unordered_map<c4c::ValueNameId, RegisterOperand> emitted_registers;
};

[[nodiscard]] std::optional<abi::RegisterView> scalar_register_view(
    bir::TypeKind type);
[[nodiscard]] bool is_scalar_alu_integer_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_floating_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_floating_type(bir::TypeKind type);
[[nodiscard]] ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    bir::BinaryOpcode opcode);
[[nodiscard]] std::optional<ImmediateOperand> make_scalar_immediate_operand(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id = std::nullopt,
    c4c::ValueNameId source_value_name = c4c::kInvalidValueName);
[[nodiscard]] PreparedScalarAluRecordError make_prepared_scalar_operand(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& value,
    OperandRecord& out);
[[nodiscard]] ScalarInstructionRecord make_scalar_alu_instruction_record(
    ScalarAluRecord alu);
[[nodiscard]] PreparedScalarAluRecordResult make_prepared_scalar_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedScalarInstructionRecordResult
make_prepared_scalar_alu_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary);
[[nodiscard]] std::optional<RegisterOperand> find_emitted_scalar_register(
    const BlockScalarLoweringState& state,
    c4c::ValueNameId value_name);
void record_emitted_scalar_register(BlockScalarLoweringState& state,
                                    c4c::ValueNameId value_name,
                                    RegisterOperand reg);
[[nodiscard]] std::optional<module::MachineInstruction> lower_scalar_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
