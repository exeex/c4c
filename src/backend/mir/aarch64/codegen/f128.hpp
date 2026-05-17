#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::aarch64::codegen {

struct LowerF128RuntimeHelperInstructionResult {
  bool handled = false;
  std::optional<module::MachineInstruction> instruction;
};

[[nodiscard]] std::string_view f128_transport_kind_name(F128TransportKind kind);
[[nodiscard]] std::string_view f128_runtime_helper_boundary_kind_name(
    F128RuntimeHelperBoundaryKind kind);
[[nodiscard]] std::string_view prepared_f128_transport_record_error_name(
    PreparedF128TransportRecordError error);
[[nodiscard]] std::string_view prepared_f128_runtime_helper_record_error_name(
    PreparedF128RuntimeHelperRecordError error);
[[nodiscard]] std::optional<std::string> f128_vector_register_name(
    const RegisterOperand& operand);

[[nodiscard]] InstructionRecord make_f128_transport_instruction(
    F128TransportRecord instruction);
[[nodiscard]] InstructionRecord make_f128_runtime_helper_boundary_instruction(
    F128RuntimeHelperBoundaryRecord instruction);
[[nodiscard]] mir::TargetInstructionPrintResult print_f128_transport(
    const InstructionRecord& instruction,
    const F128TransportRecord& transport);
[[nodiscard]] mir::TargetInstructionPrintResult print_f128_runtime_helper(
    const InstructionRecord& instruction,
    const F128RuntimeHelperBoundaryRecord& helper);

[[nodiscard]] PreparedF128TransportRecordResult make_prepared_f128_carrier_transport_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    c4c::ValueNameId value_name,
    F128TransportKind transport_kind,
    std::optional<MemoryOperand> memory);
[[nodiscard]] PreparedF128RuntimeHelperRecordResult
make_prepared_f128_runtime_helper_boundary_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    const prepare::PreparedF128RuntimeHelper& helper);

[[nodiscard]] LowerF128RuntimeHelperInstructionResult
lower_f128_runtime_helper_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
