#pragma once

#include "../abi/abi.hpp"
#include "../codegen/records.hpp"
#include "../../mir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::module {

namespace prepare = c4c::backend::prepare;
namespace abi = c4c::backend::aarch64::abi;
namespace codegen = c4c::backend::aarch64::codegen;

using MachineInstruction =
    c4c::backend::mir::MachineInstruction<codegen::InstructionRecord>;
using MachineBlock = c4c::backend::mir::MachineBlock<codegen::InstructionRecord>;
using MachineFunction =
    c4c::backend::mir::MachineFunction<codegen::InstructionRecord>;
using MachineModule = c4c::backend::mir::MachineModule<codegen::InstructionRecord>;

struct ModuleDataRecords {
};

enum class OperandAuthority {
  None,
  RegallocAssignment,
  StoragePlan,
  PreparedValueHome,
  FrameSlot,
  SpillSlot,
  Immediate,
  Symbol,
  Label,
  TargetRegister,
};

enum class InstructionLoweringFamily {
  Unknown,
  Phi,
  Scalar,
  Select,
  Memory,
  Call,
  BranchControl,
};

enum class ModuleLoweringDiagnosticKind {
  MissingFunctionContext,
  MissingValueAuthority,
  MissingTypedRegisterAuthority,
  RegisterConversionFailed,
  UnsupportedValueHome,
  UnsupportedStoragePlan,
  MissingBlockContext,
  MissingInstructionBlockMapping,
  UnsupportedInstructionFamily,
  UnsupportedTerminatorFamily,
};

struct ModuleLoweringDiagnostic {
  ModuleLoweringDiagnosticKind kind = ModuleLoweringDiagnosticKind::MissingValueAuthority;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::optional<std::size_t> instruction_index;
  InstructionLoweringFamily instruction_family = InstructionLoweringFamily::Unknown;
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string message;
};

struct ModuleLoweringDiagnostics {
  std::vector<ModuleLoweringDiagnostic> entries;

  [[nodiscard]] bool empty() const { return entries.empty(); }
};

struct FunctionLoweringContext {
  const prepare::PreparedBirModule* prepared = nullptr;
  const c4c::TargetProfile* target_profile = nullptr;
  const prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const c4c::backend::bir::Function* bir_function = nullptr;
  const prepare::PreparedValueLocationFunction* value_locations = nullptr;
  const prepare::PreparedStoragePlanFunction* storage_plan = nullptr;
  const prepare::PreparedRegallocFunction* regalloc = nullptr;
};

struct BlockLoweringContext {
  FunctionLoweringContext function;
  const prepare::PreparedControlFlowBlock* control_flow_block = nullptr;
  const c4c::backend::bir::Block* bir_block = nullptr;
  std::size_t block_index = 0;
};

struct InstructionDispatchResult {
  std::size_t visited_operations = 0;
  bool visited_terminator = false;
  std::size_t emitted_instructions = 0;
};

struct ResolvedOperand {
  c4c::backend::mir::Operand operand;
  OperandAuthority authority = OperandAuthority::None;
  std::optional<abi::RegisterReference> register_reference;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::string_view label;
  MachineFunction mir;
  std::vector<codegen::InstructionRecord> machine_nodes;
};

struct CompatibilityProjection {
  std::vector<FunctionRecord> functions;
};

struct Module {
  const prepare::PreparedBirModule* prepared = nullptr;
  c4c::TargetProfile target_profile{};
  MachineModule mir;
  ModuleDataRecords data;
  CompatibilityProjection compatibility;
  std::vector<FunctionRecord> functions;
};

struct BuildResult {
  std::optional<Module> module;
  std::optional<abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const prepare::PreparedBirModule& prepared);
[[nodiscard]] std::vector<MachineFunction> lower_prepared_functions(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] FunctionLoweringContext make_function_lowering_context(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    const prepare::PreparedControlFlowFunction& function);
[[nodiscard]] BlockLoweringContext make_block_lowering_context(
    FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index);
[[nodiscard]] InstructionDispatchResult dispatch_prepared_block(
    const BlockLoweringContext& context,
    MachineBlock& block,
    ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<MachineInstruction> lower_prepared_branch_terminator(
    const BlockLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<ResolvedOperand> resolve_value_operand(
    prepare::PreparedValueId value_id,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] ResolvedOperand resolve_immediate_operand(
    c4c::backend::mir::Immediate immediate);
[[nodiscard]] ResolvedOperand resolve_label_operand(c4c::BlockLabelId label);
[[nodiscard]] ResolvedOperand resolve_symbol_operand(c4c::LinkNameId symbol,
                                                     std::int64_t addend = 0);

}  // namespace c4c::backend::aarch64::module
