#include "private.hpp"

#include <optional>
#include <sstream>
#include <string_view>

namespace c4c::backend::prepare {

namespace addressing_printer {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string_view maybe_text_name(const PreparedNameTables& names, std::optional<TextId> id) {
  if (!id.has_value() || *id == kInvalidText) {
    return "<none>";
  }
  const std::string_view spelling = names.texts.lookup(*id);
  return spelling.empty() ? std::string_view{"<none>"} : spelling;
}

std::string_view address_space_name(bir::AddressSpace address_space) {
  switch (address_space) {
    case bir::AddressSpace::Default:
      return "default";
    case bir::AddressSpace::Fs:
      return "fs";
    case bir::AddressSpace::Gs:
      return "gs";
    case bir::AddressSpace::Tls:
      return "tls";
  }
  return "unknown";
}

}  // namespace addressing_printer

void append_addressing(std::ostringstream& out, const PreparedBirModule& module) {
  using namespace addressing_printer;

  out << "--- prepared-addressing ---\n";
  for (const auto& function_addressing : module.addressing.functions) {
    out << "prepared.func @" << maybe_function_name(module.names, function_addressing.function_name)
        << " frame_size=" << function_addressing.frame_size_bytes
        << " frame_alignment=" << function_addressing.frame_alignment_bytes << "\n";
    for (const auto& access : function_addressing.accesses) {
      out << "  access block=" << maybe_block_label(module.names, access.block_label)
          << " inst_index=" << access.inst_index
          << " base=" << prepared_address_base_kind_name(access.address.base_kind);
      if (access.result_value_name.has_value()) {
        out << " result=" << prepared_value_name(module.names, *access.result_value_name);
      }
      if (access.stored_value_name.has_value()) {
        out << " stored=" << prepared_value_name(module.names, *access.stored_value_name);
      }
      if (access.address.frame_slot_id.has_value()) {
        out << " frame_slot=#" << *access.address.frame_slot_id;
      }
      if (access.address.symbol_name.has_value()) {
        out << " symbol=" << prepared_link_name(module.names, *access.address.symbol_name);
      }
      if (access.address.pointer_value_name.has_value()) {
        out << " pointer=" << prepared_value_name(module.names, *access.address.pointer_value_name);
      }
      out << " offset=" << access.address.byte_offset
          << " size=" << access.address.size_bytes
          << " align=" << access.address.align_bytes
          << " base_plus_offset="
          << (access.address.can_use_base_plus_offset ? "yes" : "no")
          << "\n";
    }
    for (const auto& materialization : function_addressing.address_materializations) {
      out << "  address_materialization block="
          << maybe_block_label(module.names, materialization.block_label)
          << " inst_index=" << materialization.inst_index
          << " kind="
          << prepared_address_materialization_kind_name(materialization.kind);
      if (materialization.result_value_name.has_value()) {
        out << " result="
            << prepared_value_name(module.names, *materialization.result_value_name);
      }
      if (materialization.result_value_id.has_value()) {
        out << " result_id=#" << *materialization.result_value_id;
      }
      if (materialization.result_home_kind.has_value()) {
        out << " home="
            << prepared_value_home_kind_name(*materialization.result_home_kind);
      }
      if (materialization.symbol_name.has_value()) {
        out << " symbol="
            << prepared_link_name(module.names, *materialization.symbol_name);
      }
      if (materialization.text_name.has_value()) {
        out << " text=" << maybe_text_name(module.names, materialization.text_name);
      }
      if (materialization.target_label.has_value()) {
        out << " target_label="
            << maybe_block_label(module.names, *materialization.target_label);
      }
      out << " policy="
          << bir::global_address_materialization_policy_name(
                 materialization.address_materialization_policy);
      out << " offset=" << materialization.byte_offset
          << " address_space=" << address_space_name(materialization.address_space)
          << " tls_global=" << (materialization.is_thread_local ? "yes" : "no")
          << " tls_address_space="
          << (materialization.has_tls_address_space ? "yes" : "no");
      if (materialization.kind == PreparedAddressMaterializationKind::TlsGlobal ||
          materialization.tls_model != PreparedTlsMaterializationModel::None ||
          materialization.tls_thread_pointer_register != PreparedTlsThreadPointerRegister::None ||
          materialization.tls_high_relocation != PreparedTlsRelocationKind::None ||
          materialization.tls_low_relocation != PreparedTlsRelocationKind::None) {
        out << " tls_model="
            << prepared_tls_materialization_model_name(materialization.tls_model)
            << " tls_thread_pointer="
            << prepared_tls_thread_pointer_register_name(
                   materialization.tls_thread_pointer_register)
            << " tls_high_reloc="
            << prepared_tls_relocation_kind_name(materialization.tls_high_relocation)
            << " tls_low_reloc="
            << prepared_tls_relocation_kind_name(materialization.tls_low_relocation);
      }
      out << "\n";
    }
  }
}

}  // namespace c4c::backend::prepare
