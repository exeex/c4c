#include "bir.hpp"

#include <iomanip>
#include <sstream>
#include <type_traits>

namespace c4c::backend::bir {

namespace {

std::string escape_quoted_text(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

std::string render_value(const Value& value) {
  // Printer output is dump/final spelling only. Semantic lookup has already
  // happened before values reach this surface.
  if (value.kind == Value::Kind::Named) {
    return value.name;
  }
  if (value.type == TypeKind::F32 || value.type == TypeKind::F64) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0');
    if (value.type == TypeKind::F32) {
      out << std::setw(8) << static_cast<std::uint32_t>(value.immediate_bits);
    } else {
      out << std::setw(16) << value.immediate_bits;
    }
    return out.str();
  }
  return std::to_string(value.immediate);
}

std::string render_call_type_name(const CallInst& call,
                                  const StructuredTypeSpellingContext& structured_types) {
  if (call.structured_return_type_name.has_value() &&
      structured_types.find_struct_decl(*call.structured_return_type_name) != nullptr) {
    if (call.result_abi.has_value() && call.result_abi->returned_in_memory) {
      return "void";
    }
    if (call.return_type == TypeKind::Void) {
      return "void";
    }
    return *call.structured_return_type_name;
  }
  if (!call.return_type_name.empty()) {
    return call.return_type_name;
  }
  if (call.result.has_value()) {
    return render_type(call.result->type);
  }
  return "void";
}

std::string render_call_target(const CallInst& call) {
  // Call target text is dump/final spelling. Direct-call semantic identity is
  // carried by callee_link_name_id when available.
  if (call.is_indirect && call.callee_value.has_value()) {
    return render_value(*call.callee_value);
  }
  return call.callee;
}

void render_byval_suffix(std::ostringstream& out, std::size_t size_bytes, std::size_t align_bytes) {
  out << " byval";
  if (size_bytes != 0 || align_bytes != 0) {
    out << "(";
    bool wrote_field = false;
    if (size_bytes != 0) {
      out << "size=" << size_bytes;
      wrote_field = true;
    }
    if (align_bytes != 0) {
      if (wrote_field) {
        out << ", ";
      }
      out << "align=" << align_bytes;
    }
    out << ")";
  }
}

void render_sret_suffix(std::ostringstream& out, std::size_t size_bytes, std::size_t align_bytes) {
  out << " sret";
  if (size_bytes != 0 || align_bytes != 0) {
    out << "(";
    bool wrote_field = false;
    if (size_bytes != 0) {
      out << "size=" << size_bytes;
      wrote_field = true;
    }
    if (align_bytes != 0) {
      if (wrote_field) {
        out << ", ";
      }
      out << "align=" << align_bytes;
    }
    out << ")";
  }
}

std::string render_block_label(const NameTables& names,
                               BlockLabelId label_id,
                               const std::string& fallback) {
  // Prefer structured label spelling for dumps; fallback is raw-only
  // compatibility text for legacy BIR payloads.
  const std::string_view spelling = names.block_labels.spelling(label_id);
  if (!spelling.empty()) {
    return std::string(spelling);
  }
  return fallback;
}

void render_memory_address(std::ostringstream& out,
                           const MemoryAddress& address,
                           const NameTables& names) {
  out << ", addr ";
  switch (address.base_kind) {
    case MemoryAddress::BaseKind::LocalSlot:
    case MemoryAddress::BaseKind::GlobalSymbol:
    case MemoryAddress::BaseKind::StringConstant:
      out << address.base_name;
      break;
    case MemoryAddress::BaseKind::Label:
      out << render_block_label(names, address.base_label_id, address.base_name);
      break;
    case MemoryAddress::BaseKind::PointerValue:
      out << render_value(address.base_value);
      break;
    case MemoryAddress::BaseKind::None:
      out << "<none>";
      break;
  }
  if (address.byte_offset != 0) {
    out << (address.byte_offset > 0 ? "+" : "") << address.byte_offset;
  }
}

void render_phi_observation(std::ostringstream& out,
                            const PhiObservation& observation,
                            const NameTables& names) {
  out << "; semantic_phi " << observation.result.name << " = bir.phi "
      << render_type(observation.result.type);
  for (const auto& incoming : observation.incomings) {
    out << " [" << render_block_label(names, incoming.label_id, incoming.label) << ", "
        << render_value(incoming.value) << "]";
  }
  out << "\n";
}

void render_function(std::ostringstream& out,
                     const Function& function,
                     const StructuredTypeSpellingContext& structured_types,
                     const NameTables& names) {
  out << "bir.func @" << function.name << "(";
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    if (index != 0) {
      out << ", ";
    }
    out << render_type(function.params[index].type);
    if (function.params[index].is_sret) {
      render_sret_suffix(
          out, function.params[index].size_bytes, function.params[index].align_bytes);
    } else if (function.params[index].is_byval) {
      render_byval_suffix(
          out, function.params[index].size_bytes, function.params[index].align_bytes);
    }
    out << " " << function.params[index].name;
  }
  out << ") -> " << render_type(function.return_type);
  if (function.is_declaration) {
    out << "\n";
    return;
  }

  out << " {\n";
  for (const auto& block : function.blocks) {
    out << render_block_label(names, block.label_id, block.label) << ":\n";
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
            } else if constexpr (std::is_same_v<T, PhiInst>) {
              out << "  " << lowered.result.name << " = bir.phi "
                  << render_type(lowered.result.type);
              for (const auto& incoming : lowered.incomings) {
                out << " [" << render_block_label(names, incoming.label_id, incoming.label)
                    << ", " << render_value(incoming.value) << "]";
              }
              out << "\n";
            } else if constexpr (std::is_same_v<T, CallInst>) {
              if (lowered.result.has_value()) {
                out << "  " << lowered.result->name << " = ";
              } else {
                out << "  ";
              }
              out << "bir.call " << render_call_type_name(lowered, structured_types) << " "
                  << render_call_target(lowered) << "(";
              for (std::size_t arg_index = 0; arg_index < lowered.args.size(); ++arg_index) {
                if (arg_index != 0) {
                  out << ", ";
                }
                out << render_type(lowered.args[arg_index].type);
                if (arg_index < lowered.arg_abi.size()) {
                  if (lowered.arg_abi[arg_index].sret_pointer) {
                    render_sret_suffix(
                        out,
                        lowered.arg_abi[arg_index].size_bytes,
                        lowered.arg_abi[arg_index].align_bytes);
                  } else if (lowered.arg_abi[arg_index].byval_copy) {
                    render_byval_suffix(
                        out,
                        lowered.arg_abi[arg_index].size_bytes,
                        lowered.arg_abi[arg_index].align_bytes);
                  }
                }
                out << " " << render_value(lowered.args[arg_index]);
              }
              out << ")";
              if (lowered.inline_asm.has_value()) {
                out << " [asm=\"" << escape_quoted_text(lowered.inline_asm->asm_text)
                    << "\", constraints=\""
                    << escape_quoted_text(lowered.inline_asm->constraints) << "\"";
                if (!lowered.inline_asm->args_text.empty()) {
                  out << ", raw_args=\""
                      << escape_quoted_text(lowered.inline_asm->args_text) << "\"";
                }
                if (lowered.inline_asm->side_effects) {
                  out << ", sideeffect";
                }
                out << "]";
              }
              out << "\n";
            } else if constexpr (std::is_same_v<T, LoadLocalInst>) {
              out << "  " << lowered.result.name << " = bir.load_local "
                  << render_type(lowered.result.type) << " " << lowered.slot_name;
              if (lowered.address.has_value()) {
                render_memory_address(out, *lowered.address, names);
              }
              out << "\n";
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
                  << render_type(lowered.value.type) << " " << render_value(lowered.value);
              if (lowered.address.has_value()) {
                render_memory_address(out, *lowered.address, names);
              }
              out << "\n";
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
        out << "bir.br "
            << render_block_label(
                   names, block.terminator.target_label_id, block.terminator.target_label);
        break;
      case TerminatorKind::CondBranch:
        out << "bir.cond_br " << render_type(block.terminator.condition.type) << " "
            << render_value(block.terminator.condition) << ", "
            << render_block_label(names,
                                  block.terminator.true_label_id,
                                  block.terminator.true_label)
            << ", "
            << render_block_label(names,
                                  block.terminator.false_label_id,
                                  block.terminator.false_label);
        break;
    }
    out << "\n";
  }
  out << "}\n";
  for (const auto& slot : function.local_slots) {
    if (slot.phi_observation.has_value()) {
      render_phi_observation(out, *slot.phi_observation, names);
    }
  }
}

}  // namespace

std::string print(const Module& module) {
  std::ostringstream out;
  for (std::size_t index = 0; index < module.functions.size(); ++index) {
    if (index != 0) {
      out << "\n";
    }
    render_function(out, module.functions[index], module.structured_types, module.names);
  }
  return out.str();
}

}  // namespace c4c::backend::bir
