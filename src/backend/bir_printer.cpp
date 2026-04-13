#include "bir_printer.hpp"

#include <sstream>
#include <type_traits>

namespace c4c::backend::bir {

namespace {

std::string render_value(const Value& value) {
  if (value.kind == Value::Kind::Named) {
    return value.name;
  }
  return std::to_string(value.immediate);
}

std::string render_call_type_name(const CallInst& call) {
  if (!call.return_type_name.empty()) {
    return call.return_type_name;
  }
  if (call.result.has_value()) {
    return render_type(call.result->type);
  }
  return "void";
}

std::string render_call_target(const CallInst& call) {
  if (call.is_indirect && call.callee_value.has_value()) {
    return render_value(*call.callee_value);
  }
  return call.callee;
}

void render_function(std::ostringstream& out, const Function& function) {
  out << "bir.func @" << function.name << "(";
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    if (index != 0) {
      out << ", ";
    }
    out << render_type(function.params[index].type) << " "
        << function.params[index].name;
  }
  out << ") -> " << render_type(function.return_type);
  if (function.is_declaration) {
    out << "\n";
    return;
  }

  out << " {\n";
  for (const auto& block : function.blocks) {
    out << block.label << ":\n";
    for (const auto& inst : block.insts) {
      std::visit(
          [&](const auto& lowered) {
            using T = std::decay_t<decltype(lowered)>;
            if constexpr (std::is_same_v<T, BinaryInst>) {
              out << "  " << lowered.result.name << " = bir."
                  << render_binary_opcode(lowered.opcode) << " "
                  << render_type(binary_operand_type(lowered)) << " "
                  << render_value(lowered.lhs) << ", " << render_value(lowered.rhs)
                  << "\n";
            } else if constexpr (std::is_same_v<T, SelectInst>) {
              out << "  " << lowered.result.name << " = bir.select "
                  << render_binary_opcode(lowered.predicate) << " "
                  << render_type(select_compare_type(lowered)) << " "
                  << render_value(lowered.lhs) << ", " << render_value(lowered.rhs)
                  << ", " << render_type(lowered.result.type) << " "
                  << render_value(lowered.true_value) << ", "
                  << render_value(lowered.false_value) << "\n";
            } else if constexpr (std::is_same_v<T, CastInst>) {
              out << "  " << lowered.result.name << " = bir."
                  << render_cast_opcode(lowered.opcode) << " "
                  << render_type(lowered.operand.type) << " "
                  << render_value(lowered.operand) << " to "
                  << render_type(lowered.result.type) << "\n";
            } else if constexpr (std::is_same_v<T, CallInst>) {
              if (lowered.result.has_value()) {
                out << "  " << lowered.result->name << " = ";
              } else {
                out << "  ";
              }
              out << "bir.call " << render_call_type_name(lowered) << " "
                  << render_call_target(lowered) << "(";
              for (std::size_t arg_index = 0; arg_index < lowered.args.size(); ++arg_index) {
                if (arg_index != 0) {
                  out << ", ";
                }
                out << render_type(lowered.args[arg_index].type) << " "
                    << render_value(lowered.args[arg_index]);
              }
              out << ")\n";
            } else if constexpr (std::is_same_v<T, LoadLocalInst>) {
              out << "  " << lowered.result.name << " = bir.load_local "
                  << render_type(lowered.result.type) << " " << lowered.slot_name << "\n";
            } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
              out << "  " << lowered.result.name << " = bir.load_global "
                  << render_type(lowered.result.type) << " @" << lowered.global_name;
              if (lowered.byte_offset != 0) {
                out << ", offset " << lowered.byte_offset;
              }
              out << "\n";
            } else if constexpr (std::is_same_v<T, StoreGlobalInst>) {
              out << "  bir.store_global @" << lowered.global_name;
              if (lowered.byte_offset != 0) {
                out << ", offset " << lowered.byte_offset;
              }
              out << ", " << render_type(lowered.value.type) << " "
                  << render_value(lowered.value) << "\n";
            } else if constexpr (std::is_same_v<T, StoreLocalInst>) {
              out << "  bir.store_local " << lowered.slot_name << ", "
                  << render_type(lowered.value.type) << " " << render_value(lowered.value)
                  << "\n";
            }
          },
          inst);
    }
    out << "  ";
    switch (block.terminator.kind) {
      case TerminatorKind::Return:
        out << "bir.ret";
        if (block.terminator.value.has_value()) {
          out << " " << render_type(block.terminator.value->type) << " "
              << render_value(*block.terminator.value);
        }
        break;
      case TerminatorKind::Branch:
        out << "bir.br " << block.terminator.target_label;
        break;
      case TerminatorKind::CondBranch:
        out << "bir.cond_br " << render_type(block.terminator.condition.type) << " "
            << render_value(block.terminator.condition) << ", "
            << block.terminator.true_label << ", " << block.terminator.false_label;
        break;
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
