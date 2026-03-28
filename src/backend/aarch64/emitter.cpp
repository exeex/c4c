#include "emitter.hpp"

#include "alu.hpp"
#include "branch.hpp"
#include "calls.hpp"
#include "frame.hpp"
#include "memory.hpp"
#include "support.hpp"

#include <sstream>

namespace c4c::backend::aarch64 {

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  validate_module(module);

  std::ostringstream out;
  render_module_header(out, module);

  for (const auto& function : module.functions) {
    validate_function(function);
    render_function_prologue(out, function);
    if (function.is_declaration) {
      continue;
    }

    for (const auto& block : function.blocks) {
      out << block.label << ":\n";
      if (block.id.value == function.entry.value) {
        render_entry_allocas(out, function);
      }
      for (const auto& inst : block.insts) {
        if (render_alu_instruction(out, inst)) {
          continue;
        }
        if (render_branch_instruction(out, inst)) {
          continue;
        }
        if (render_call_instruction(out, inst)) {
          continue;
        }
        if (render_memory_instruction(out, inst)) {
          continue;
        }
        fail_unsupported("non-ALU/non-branch/non-call/non-memory instructions");
      }
      render_terminator(out, block.terminator);
    }

    render_function_epilogue(out);
  }

  return out.str();
}

}  // namespace c4c::backend::aarch64
