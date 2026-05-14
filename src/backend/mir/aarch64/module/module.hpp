#pragma once

#include "../abi/abi.hpp"
#include "../codegen/records.hpp"
#include "../../mir.hpp"

#include <optional>
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

}  // namespace c4c::backend::aarch64::module
