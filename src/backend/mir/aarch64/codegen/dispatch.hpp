#pragma once

#include "../module/module.hpp"

#include <cstddef>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;

struct InstructionDispatchResult {
  std::size_t visited_operations = 0;
  bool visited_terminator = false;
  std::size_t emitted_instructions = 0;
};

[[nodiscard]] module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index);
[[nodiscard]] InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
