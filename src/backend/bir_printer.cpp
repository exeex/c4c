#include "bir_printer.hpp"

#include <sstream>

namespace c4c::backend::bir {

namespace {

std::string render_value(const Value& value) {
  if (value.kind == Value::Kind::Named) {
    return value.name;
  }
  return std::to_string(value.immediate);
}

void render_function(std::ostringstream& out, const Function& function) {
  out << "bir.func @" << function.name << "() -> "
      << render_type(function.return_type);
  if (function.is_declaration) {
    out << "\n";
    return;
  }

  out << " {\n";
  for (const auto& block : function.blocks) {
    out << block.label << ":\n";
    for (const auto& inst : block.insts) {
      out << "  " << inst.result.name << " = bir."
          << render_binary_opcode(inst.opcode) << " "
          << render_type(inst.result.type) << " "
          << render_value(inst.lhs) << ", " << render_value(inst.rhs) << "\n";
    }
    out << "  bir.ret";
    if (block.terminator.value.has_value()) {
      out << " " << render_type(block.terminator.value->type) << " "
          << render_value(*block.terminator.value);
    }
    out << "\n";
  }
  out << "}\n";
}

}  // namespace

std::string print(const Module& module) {
  std::ostringstream out;
  for (std::size_t index = 0; index < module.functions.size(); ++index) {
    if (index != 0) {
      out << "\n";
    }
    render_function(out, module.functions[index]);
  }
  return out.str();
}

}  // namespace c4c::backend::bir
