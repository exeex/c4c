#include "ir_printer.hpp"

#include "../codegen/lir/call_args.hpp"

#include <sstream>

namespace c4c::backend {

namespace {

void render_signature(std::ostringstream& out,
                      const BackendFunctionSignature& signature) {
  out << signature.linkage << " " << signature.return_type << " @"
      << signature.name << "(";
  bool first = true;
  for (const auto& param : signature.params) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << param.type_str;
    if (!param.name.empty()) {
      out << " " << param.name;
    }
  }
  if (signature.is_vararg) {
    if (!first) {
      out << ", ";
    }
    out << "...";
  }
  out << ")\n";
}

void render_inst(std::ostringstream& out, const BackendInst& inst) {
  if (const auto* bin = std::get_if<BackendBinaryInst>(&inst)) {
    out << "  " << bin->result << " = " << backend_binary_opcode_name(bin->opcode)
        << " " << bin->type_str << " " << bin->lhs << ", " << bin->rhs << "\n";
    return;
  }

  const auto* call = std::get_if<BackendCallInst>(&inst);
  if (call == nullptr) {
    return;
  }

  out << "  ";
  if (!call->result.empty()) {
    out << call->result << " = ";
  }
  out << "call " << call->return_type << " ";
  const auto callee_type_suffix =
      call->render_callee_type_suffix
          ? c4c::codegen::lir::format_lir_call_param_types(call->param_types)
          : std::string{};
  out << c4c::codegen::lir::format_lir_call_site(
             call->callee,
             callee_type_suffix,
             c4c::codegen::lir::format_lir_typed_call_args(call->args))
      << "\n";
}

void render_function(std::ostringstream& out, const BackendFunction& function) {
  render_signature(out, function.signature);
  if (function.is_declaration) {
    return;
  }

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

std::string print_backend_ir(const BackendModule& module) {
  std::ostringstream out;
  if (!module.data_layout.empty()) {
    out << "target datalayout = \"" << module.data_layout << "\"\n";
  }
  if (!module.target_triple.empty()) {
    out << "target triple = \"" << module.target_triple << "\"\n";
  }
  if (!module.data_layout.empty() || !module.target_triple.empty()) {
    out << "\n";
  }
  for (const auto& type_decl : module.type_decls) {
    out << type_decl << "\n";
  }
  if (!module.type_decls.empty()) {
    out << "\n";
  }
  for (const auto& function : module.functions) {
    render_function(out, function);
  }
  return out.str();
}

}  // namespace c4c::backend
