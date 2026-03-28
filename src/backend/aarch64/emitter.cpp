#include "emitter.hpp"

#include "alu.hpp"
#include "branch.hpp"
#include "calls.hpp"
#include "frame.hpp"
#include "memory.hpp"
#include "prologue.hpp"
#include "support.hpp"

#include <sstream>

namespace c4c::backend::aarch64 {

namespace {

const char* instruction_kind(const c4c::codegen::lir::LirInst& inst) {
  if (std::holds_alternative<c4c::codegen::lir::LirAllocaOp>(inst)) return "alloca";
  if (std::holds_alternative<c4c::codegen::lir::LirLoadOp>(inst)) return "load";
  if (std::holds_alternative<c4c::codegen::lir::LirStoreOp>(inst)) return "store";
  if (std::holds_alternative<c4c::codegen::lir::LirMemcpyOp>(inst)) return "memcpy";
  if (std::holds_alternative<c4c::codegen::lir::LirCastOp>(inst)) return "cast";
  if (std::holds_alternative<c4c::codegen::lir::LirBinOp>(inst)) return "binop";
  if (std::holds_alternative<c4c::codegen::lir::LirCmpOp>(inst)) return "cmp";
  if (std::holds_alternative<c4c::codegen::lir::LirGepOp>(inst)) return "gep";
  if (std::holds_alternative<c4c::codegen::lir::LirCallOp>(inst)) return "call";
  if (std::holds_alternative<c4c::codegen::lir::LirVaStartOp>(inst)) return "va_start";
  if (std::holds_alternative<c4c::codegen::lir::LirVaEndOp>(inst)) return "va_end";
  if (std::holds_alternative<c4c::codegen::lir::LirVaCopyOp>(inst)) return "va_copy";
  if (std::holds_alternative<c4c::codegen::lir::LirVaArgOp>(inst)) return "va_arg";
  if (std::holds_alternative<c4c::codegen::lir::LirPhiOp>(inst)) return "phi";
  return "unknown";
}

}  // namespace

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
        fail_unsupported("non-ALU/non-branch/non-call/non-memory instructions (saw " +
                         std::string(instruction_kind(inst)) + ")");
      }
      render_terminator(out, block.terminator);
    }

    render_function_epilogue(out);
  }

  return out.str();
}

}  // namespace c4c::backend::aarch64
