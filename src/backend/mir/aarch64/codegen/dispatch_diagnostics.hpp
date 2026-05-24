#pragma once

#include "../module/module.hpp"
#include "../../query.hpp"

#include <cstddef>
#include <string>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message);

[[nodiscard]] std::string unsupported_terminator_message(bir::TerminatorKind kind);

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index);

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message);

}  // namespace c4c::backend::aarch64::codegen
