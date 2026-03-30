#include "ir_printer.hpp"

#include "../codegen/lir/call_args.hpp"

#include <sstream>
#include <string_view>

namespace c4c::backend {

namespace {

std::string escape_ir_bytes(std::string_view bytes) {
  static constexpr char kHexDigits[] = "0123456789ABCDEF";
  std::string escaped;
  escaped.reserve(bytes.size() * 2);
  for (unsigned char ch : bytes) {
    if (ch >= 0x20 && ch <= 0x7e && ch != '"' && ch != '\\') {
      escaped.push_back(static_cast<char>(ch));
      continue;
    }
    escaped.push_back('\\');
    escaped.push_back(kHexDigits[(ch >> 4) & 0xf]);
    escaped.push_back(kHexDigits[ch & 0xf]);
  }
  return escaped;
}

void render_global(std::ostringstream& out, const BackendGlobal& global) {
  out << "@" << global.name << " = " << global.linkage << global.qualifier
      << global.llvm_type;
  if (!global.is_extern_decl) {
    out << " " << global.init_text;
  }
  if (global.align_bytes > 1) {
    out << ", align " << global.align_bytes;
  }
  out << "\n";
}

void render_string_constant(std::ostringstream& out,
                            const BackendStringConstant& string_constant) {
  out << "@" << string_constant.name << " = private unnamed_addr constant ["
      << string_constant.byte_length << " x i8] c\""
      << escape_ir_bytes(string_constant.raw_bytes) << "\\00\"\n";
}

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

  if (const auto* cmp = std::get_if<BackendCompareInst>(&inst)) {
    out << "  " << cmp->result << " = icmp "
        << backend_compare_predicate_name(cmp->predicate) << " "
        << cmp->type_str << " " << cmp->lhs << ", " << cmp->rhs << "\n";
    return;
  }

  const auto* call = std::get_if<BackendCallInst>(&inst);
  if (call == nullptr) {
    const auto* load = std::get_if<BackendLoadInst>(&inst);
    if (load != nullptr) {
      out << "  " << load->result << " = load ";
      if (!load->memory_type.empty() && load->memory_type != load->type_str) {
        out << load->type_str << " from " << load->memory_type;
        if (load->extension == BackendLoadExtension::SignExtend) {
          out << " sext";
        } else if (load->extension == BackendLoadExtension::ZeroExtend) {
          out << " zext";
        }
      } else {
        out << load->type_str;
      }
      out << ", ptr @" << load->address.base_symbol;
      if (load->address.byte_offset != 0) {
        out << " + " << load->address.byte_offset;
      }
      out << "\n";
      return;
    }

    const auto* store = std::get_if<BackendStoreInst>(&inst);
    if (store != nullptr) {
      out << "  store " << store->type_str << " " << store->value << ", ptr @"
          << store->address.base_symbol;
      if (store->address.byte_offset != 0) {
        out << " + " << store->address.byte_offset;
      }
      out << "\n";
      return;
    }

    const auto* ptrdiff = std::get_if<BackendPtrDiffEqInst>(&inst);
    if (ptrdiff == nullptr) {
      return;
    }
    out << "  " << ptrdiff->result << " = ptrdiff_eq " << ptrdiff->type_str
        << ", ptr @" << ptrdiff->lhs_address.base_symbol;
    if (ptrdiff->lhs_address.byte_offset != 0) {
      out << " + " << ptrdiff->lhs_address.byte_offset;
    }
    out << ", ptr @" << ptrdiff->rhs_address.base_symbol;
    if (ptrdiff->rhs_address.byte_offset != 0) {
      out << " + " << ptrdiff->rhs_address.byte_offset;
    }
    out << ", elem_size " << ptrdiff->element_size << ", expected "
        << ptrdiff->expected_diff << "\n";
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
    switch (block.terminator.kind) {
      case BackendTerminatorKind::Return:
        if (block.terminator.value.has_value()) {
          out << "  ret " << block.terminator.type_str << " "
              << *block.terminator.value << "\n";
        } else {
          out << "  ret void\n";
        }
        break;
      case BackendTerminatorKind::Branch:
        out << "  br label %" << block.terminator.target_label << "\n";
        break;
      case BackendTerminatorKind::CondBranch:
        out << "  br i1 " << block.terminator.cond_name << ", label %"
            << block.terminator.true_label << ", label %"
            << block.terminator.false_label << "\n";
        break;
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
  for (const auto& global : module.globals) {
    render_global(out, global);
  }
  for (const auto& string_constant : module.string_constants) {
    render_string_constant(out, string_constant);
  }
  if (!module.globals.empty() || !module.string_constants.empty()) {
    out << "\n";
  }
  for (const auto& function : module.functions) {
    render_function(out, function);
  }
  return out.str();
}

}  // namespace c4c::backend
