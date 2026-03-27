#include "lir_adapter.hpp"

#include <sstream>
#include <stdexcept>

namespace c4c::backend {

namespace {

[[noreturn]] void fail_unsupported(const std::string& detail) {
  throw std::invalid_argument("return-only LIR adapter does not support " + detail);
}

BackendBinaryInst adapt_inst(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (!bin) fail_unsupported("non-binary instructions");
  if (bin->opcode != "add") fail_unsupported("binary opcode '" + bin->opcode + "'");
  BackendBinaryInst out;
  out.opcode = BackendBinaryOpcode::Add;
  out.result = bin->result;
  out.type_str = bin->type_str;
  out.lhs = bin->lhs;
  out.rhs = bin->rhs;
  return out;
}

BackendReturn adapt_terminator(const c4c::codegen::lir::LirTerminator& terminator) {
  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&terminator);
  if (!ret) fail_unsupported("non-return terminators");
  return BackendReturn{ret->value_str, ret->type_str};
}

BackendFunction adapt_function(const c4c::codegen::lir::LirFunction& function) {
  BackendFunction out;
  out.signature_text = function.signature_text;
  out.is_declaration = function.is_declaration;
  if (function.is_declaration) {
    return out;
  }

  if (!function.stack_objects.empty()) fail_unsupported("stack objects");
  if (!function.alloca_insts.empty()) fail_unsupported("entry allocas");
  if (function.blocks.size() != 1) fail_unsupported("multi-block functions");

  const auto& block = function.blocks.front();
  BackendBlock out_block;
  out_block.label = block.label;
  for (const auto& inst : block.insts) {
    out_block.insts.push_back(adapt_inst(inst));
  }
  out_block.terminator = adapt_terminator(block.terminator);
  out.blocks.push_back(std::move(out_block));
  return out;
}

void render_inst(std::ostringstream& out, const BackendInst& inst) {
  if (const auto* bin = std::get_if<BackendBinaryInst>(&inst)) {
    const char* opcode = nullptr;
    switch (bin->opcode) {
      case BackendBinaryOpcode::Add:
        opcode = "add";
        break;
    }
    out << "  " << bin->result << " = " << opcode << " " << bin->type_str << " "
        << bin->lhs << ", " << bin->rhs << "\n";
  }
}

void render_function(std::ostringstream& out, const BackendFunction& function) {
  out << function.signature_text;
  if (function.is_declaration) return;

  out << "{\n";
  for (const auto& block : function.blocks) {
    out << block.label << ":\n";
    for (const auto& inst : block.insts) {
      render_inst(out, inst);
    }
    if (block.terminator.value.has_value()) {
      out << "  ret " << block.terminator.type_str << " "
          << *block.terminator.value << "\n";
    } else {
      out << "  ret void\n";
    }
  }
  out << "}\n\n";
}

}  // namespace

BackendModule adapt_return_only_module(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty()) fail_unsupported("globals");
  if (!module.string_pool.empty()) fail_unsupported("string constants");
  if (!module.extern_decls.empty()) fail_unsupported("extern declarations");
  if (!module.type_decls.empty()) fail_unsupported("type declarations");
  if (module.need_va_start || module.need_va_end || module.need_va_copy ||
      module.need_memcpy || module.need_stacksave || module.need_stackrestore ||
      module.need_abs) {
    fail_unsupported("intrinsic declarations");
  }

  BackendModule out;
  out.target_triple = module.target_triple;
  out.data_layout = module.data_layout;
  for (const auto& function : module.functions) {
    out.functions.push_back(adapt_function(function));
  }
  return out;
}

std::string render_module(const BackendModule& module) {
  std::ostringstream out;
  if (!module.data_layout.empty()) out << "target datalayout = \"" << module.data_layout << "\"\n";
  if (!module.target_triple.empty()) out << "target triple = \"" << module.target_triple << "\"\n";
  if (!module.data_layout.empty() || !module.target_triple.empty()) out << "\n";

  for (const auto& function : module.functions) {
    render_function(out, function);
  }
  return out.str();
}

}  // namespace c4c::backend
