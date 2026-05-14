#pragma once

#include "../abi/abi.hpp"
#include "../codegen/instruction.hpp"
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
  MissingPreparedCallPlan,
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
  const prepare::PreparedFramePlanFunction* frame_plan = nullptr;
  const prepare::PreparedDynamicStackPlanFunction* dynamic_stack_plan = nullptr;
  const prepare::PreparedCallPlansFunction* call_plans = nullptr;
};

struct BlockLoweringContext {
  FunctionLoweringContext function;
  const prepare::PreparedControlFlowBlock* control_flow_block = nullptr;
  const c4c::backend::bir::Block* bir_block = nullptr;
  std::size_t block_index = 0;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::string_view label;
  MachineFunction mir;
  // Compatibility-only flat projection of selected MIR target records. The
  // authoritative stream is `mir`; terminal assembly must not print from this
  // vector.
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
}  // namespace c4c::backend::aarch64::module
