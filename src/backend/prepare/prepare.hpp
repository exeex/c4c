#pragma once

#include "../../codegen/lir/ir.hpp"
#include "../target.hpp"

#include <string>
#include <vector>

namespace c4c::backend::prepare {

struct PrepareOptions {
  bool run_stack_layout = true;
  bool run_liveness = true;
  bool run_regalloc = true;
};

struct PrepareNote {
  std::string phase;
  std::string message;
};

struct PreparedLirModule {
  c4c::codegen::lir::LirModule module;
  Target target = Target::X86_64;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

PreparedLirModule prepare_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare
