#include "private.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

namespace c4c::backend::prepare {

namespace inline_asm_printer {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
}

std::string escape_quoted_text(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (const char ch : text) {
    if (ch == '\\' || ch == '"') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

}  // namespace inline_asm_printer

void append_inline_asm_carriers(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace inline_asm_printer;

  out << "--- prepared-inline-asm-carriers ---\n";
  for (const auto& function_carriers : module.inline_asm_carriers.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_carriers.function_name)
        << "\n";
    for (const auto& carrier : function_carriers.carriers) {
      if (carrier.carrier_kind != PreparedInlineAsmCarrierKind::Complete) {
        continue;
      }
      out << "  inline_asm_carrier asm=\"" << escape_quoted_text(carrier.asm_text)
          << "\" constraints=\"" << escape_quoted_text(carrier.constraints)
          << "\" block_index=" << carrier.block_index
          << " inst_index=" << carrier.inst_index
          << " side_effects=" << (carrier.side_effects ? "yes" : "no")
          << " operands=" << carrier.operands.size();
      if (carrier.result_value_name.has_value()) {
        out << " result=" << maybe_value_name(module.names, *carrier.result_value_name);
      }
      out << " result_home=" << (carrier.result_home.has_value() ? "yes" : "no")
          << " clobbers=" << carrier.clobbers.size();
      for (const auto& operand : carrier.operands) {
        out << " operand" << operand.constraint_index
            << "[kind=" << bir::inline_asm_operand_kind_name(operand.kind)
            << ",constraint=\"" << escape_quoted_text(operand.constraint) << "\"";
        if (operand.arg_index.has_value()) {
          out << ",arg=" << *operand.arg_index;
        }
        if (operand.output_index.has_value()) {
          out << ",output=" << *operand.output_index;
        }
        if (operand.tied_output_index.has_value()) {
          out << ",tied_output=" << *operand.tied_output_index;
        }
        if (operand.value_name.has_value()) {
          out << ",value=" << maybe_value_name(module.names, *operand.value_name);
        }
        if (operand.immediate_value.has_value()) {
          out << ",immediate=" << *operand.immediate_value;
        }
        if (operand.name.has_value()) {
          out << ",name=\"" << escape_quoted_text(*operand.name) << "\"";
        }
        if (operand.memory_address.has_value()) {
          out << ",memory_address=yes";
        }
        if (operand.address.has_value()) {
          out << ",address=yes";
        }
        out << ",home=" << (operand.home.has_value() ? "yes" : "no") << "]";
      }
      for (std::size_t index = 0; index < carrier.clobbers.size(); ++index) {
        out << " clobber" << index << "=\""
            << escape_quoted_text(carrier.clobbers[index]) << "\"";
      }
      out << "\n";
    }
    for (const auto& fact : function_carriers.missing_required_facts) {
      out << "    missing fact=" << fact << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
