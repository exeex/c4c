#pragma once

#include "instruction.hpp"
#include "../../mir.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace c4c::backend::aarch64::module {

struct BlockLoweringContext;
struct ModuleLoweringDiagnostics;

}  // namespace c4c::backend::aarch64::module

namespace c4c::backend::aarch64::codegen {

struct I128InstructionLoweringResult {
  bool handled = false;
  std::optional<c4c::backend::mir::MachineInstruction<InstructionRecord>> instruction;
};

[[nodiscard]] std::string_view i128_transport_kind_name(I128TransportKind kind);
[[nodiscard]] std::string_view prepared_i128_transport_record_error_name(
    PreparedI128TransportRecordError error);
[[nodiscard]] std::string_view i128_pair_operation_kind_name(I128PairOperationKind kind);
[[nodiscard]] std::string_view i128_pair_lane_semantics_name(
    I128PairLaneSemantics semantics);
[[nodiscard]] std::string_view i128_shift_kind_name(I128ShiftKind kind);
[[nodiscard]] std::string_view i128_shift_lane_semantics_name(
    I128ShiftLaneSemantics semantics);
[[nodiscard]] std::string_view i128_shift_count_kind_name(I128ShiftCountKind kind);
[[nodiscard]] std::string_view i128_compare_signedness_name(
    I128CompareSignedness signedness);
[[nodiscard]] std::string_view i128_compare_high_word_semantics_name(
    I128CompareHighWordSemantics semantics);
[[nodiscard]] std::string_view i128_runtime_helper_boundary_kind_name(
    I128RuntimeHelperBoundaryKind kind);
[[nodiscard]] std::string_view prepared_i128_pair_record_error_name(
    PreparedI128PairRecordError error);
[[nodiscard]] std::string_view prepared_i128_runtime_helper_record_error_name(
    PreparedI128RuntimeHelperRecordError error);

[[nodiscard]] InstructionRecord make_i128_transport_instruction(
    I128TransportRecord instruction);
[[nodiscard]] InstructionRecord make_i128_pair_operation_instruction(
    I128PairOperationRecord instruction);
[[nodiscard]] InstructionRecord make_i128_shift_instruction(I128ShiftRecord instruction);
[[nodiscard]] InstructionRecord make_i128_compare_instruction(I128CompareRecord instruction);
[[nodiscard]] InstructionRecord make_i128_runtime_helper_boundary_instruction(
    I128RuntimeHelperBoundaryRecord instruction);

[[nodiscard]] PreparedI128TransportRecordResult make_prepared_i128_carrier_transport_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    c4c::ValueNameId value_name,
    I128TransportKind transport_kind,
    std::optional<MemoryOperand> memory = std::nullopt);
[[nodiscard]] PreparedI128TransportRecordResult make_prepared_i128_copy_transport_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::CastInst& cast);
[[nodiscard]] PreparedI128PairRecordResult make_prepared_i128_pair_operation_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedI128ShiftRecordResult make_prepared_i128_shift_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedI128CompareRecordResult make_prepared_i128_compare_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedI128RuntimeHelperRecordResult
make_prepared_i128_runtime_helper_boundary_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const prepare::PreparedI128RuntimeHelper& helper);
[[nodiscard]] I128InstructionLoweringResult lower_i128_pair_operation_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] I128InstructionLoweringResult lower_i128_copy_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
