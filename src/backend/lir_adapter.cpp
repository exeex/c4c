#include "lir_adapter.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace c4c::backend {

namespace {

std::string trim(std::string text) {
  size_t start = 0;
  while (start < text.size() &&
         std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return text.substr(start, end - start);
}

std::vector<std::string> split_top_level(const std::string& text, char delim) {
  std::vector<std::string> parts;
  size_t start = 0;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    switch (text[i]) {
      case '(':
        ++paren_depth;
        break;
      case ')':
        --paren_depth;
        break;
      case '[':
        ++square_depth;
        break;
      case ']':
        --square_depth;
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        --brace_depth;
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        --angle_depth;
        break;
      default:
        break;
    }
    if (text[i] == delim && paren_depth == 0 && square_depth == 0 &&
        brace_depth == 0 && angle_depth == 0) {
      parts.push_back(trim(text.substr(start, i - start)));
      start = i + 1;
    }
  }
  parts.push_back(trim(text.substr(start)));
  return parts;
}

[[noreturn]] void fail_unsupported(const std::string& detail) {
  throw LirAdapterError(LirAdapterErrorKind::Unsupported,
                        "minimal backend LIR adapter does not support " + detail);
}

[[noreturn]] void fail_bad_signature(const std::string& signature_text) {
  throw LirAdapterError(LirAdapterErrorKind::Malformed,
                        "minimal backend LIR adapter could not parse signature '" +
                            signature_text + "'");
}

BackendFunctionSignature parse_signature(const std::string& signature_text) {
  const std::string line = trim(signature_text);
  const size_t first_space = line.find(' ');
  const size_t at_pos = line.find('@');
  const size_t open_paren = line.find('(', at_pos == std::string::npos ? 0 : at_pos);
  const size_t close_paren = line.rfind(')');
  if (first_space == std::string::npos || at_pos == std::string::npos ||
      open_paren == std::string::npos || close_paren == std::string::npos ||
      first_space >= at_pos || open_paren >= close_paren) {
    fail_bad_signature(signature_text);
  }

  BackendFunctionSignature signature;
  signature.linkage = trim(line.substr(0, first_space));
  signature.return_type = trim(line.substr(first_space + 1, at_pos - first_space - 1));
  signature.name = line.substr(at_pos + 1, open_paren - at_pos - 1);
  if (signature.linkage.empty() || signature.return_type.empty() ||
      signature.name.empty()) {
    fail_bad_signature(signature_text);
  }

  const std::string params_text =
      trim(line.substr(open_paren + 1, close_paren - open_paren - 1));
  if (!params_text.empty()) {
    for (const std::string& part : split_top_level(params_text, ',')) {
      if (part.empty()) continue;
      if (part == "...") {
        signature.is_vararg = true;
        continue;
      }
      BackendParam param;
      const size_t last_space = part.rfind(' ');
      if (last_space == std::string::npos) {
        param.type_str = part;
      } else {
        param.type_str = trim(part.substr(0, last_space));
        param.name = trim(part.substr(last_space + 1));
      }
      if (param.type_str.empty()) fail_bad_signature(signature_text);
      signature.params.push_back(std::move(param));
    }
  }

  return signature;
}

void render_signature(std::ostringstream& out,
                      const BackendFunctionSignature& signature) {
  out << signature.linkage << " " << signature.return_type << " @"
      << signature.name << "(";
  bool first = true;
  for (const auto& param : signature.params) {
    if (!first) out << ", ";
    first = false;
    out << param.type_str;
    if (!param.name.empty()) out << " " << param.name;
  }
  if (signature.is_vararg) {
    if (!first) out << ", ";
    out << "...";
  }
  out << ")\n";
}

BackendInst adapt_inst(const c4c::codegen::lir::LirInst& inst) {
  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->opcode != "add") fail_unsupported("binary opcode '" + bin->opcode + "'");
    BackendBinaryInst out;
    out.opcode = BackendBinaryOpcode::Add;
    out.result = bin->result;
    out.type_str = bin->type_str;
    out.lhs = bin->lhs;
    out.rhs = bin->rhs;
    return out;
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    BackendCallInst out;
    out.result = call->result;
    out.return_type = call->return_type;
    out.callee = call->callee;
    out.callee_type_suffix = call->callee_type_suffix;
    out.args_str = call->args_str;
    return out;
  }

  fail_unsupported("non-binary/non-call instructions");
}

BackendReturn adapt_terminator(const c4c::codegen::lir::LirTerminator& terminator) {
  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&terminator);
  if (!ret) fail_unsupported("non-return terminators");
  return BackendReturn{ret->value_str, ret->type_str};
}

BackendFunction adapt_function(const c4c::codegen::lir::LirFunction& function) {
  BackendFunction out;
  out.signature = parse_signature(function.signature_text);
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
    return;
  }

  const auto* call = std::get_if<BackendCallInst>(&inst);
  if (call == nullptr) return;

  out << "  ";
  if (!call->result.empty()) out << call->result << " = ";
  out << "call " << call->return_type << " ";
  if (!call->callee_type_suffix.empty()) out << call->callee_type_suffix << " ";
  out << call->callee << "(" << call->args_str << ")\n";
}

void render_function(std::ostringstream& out, const BackendFunction& function) {
  render_signature(out, function.signature);
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

BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty()) fail_unsupported("globals");
  if (!module.string_pool.empty()) fail_unsupported("string constants");
  if (!module.extern_decls.empty()) fail_unsupported("extern declarations");
  if (module.need_va_start || module.need_va_end || module.need_va_copy ||
      module.need_memcpy || module.need_stacksave || module.need_stackrestore ||
      module.need_abs) {
    fail_unsupported("intrinsic declarations");
  }

  BackendModule out;
  out.target_triple = module.target_triple;
  out.data_layout = module.data_layout;
  out.type_decls = module.type_decls;
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
  for (const auto& type_decl : module.type_decls) {
    out << type_decl << "\n";
  }
  if (!module.type_decls.empty()) out << "\n";

  for (const auto& function : module.functions) {
    render_function(out, function);
  }
  return out.str();
}

}  // namespace c4c::backend
