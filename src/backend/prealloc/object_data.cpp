#include "object_data.hpp"

#include "module.hpp"

#include <algorithm>
#include <utility>

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] std::optional<std::size_t> immediate_value_size_bytes(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I16:
      return 2;
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
      return 8;
    default:
      return std::nullopt;
  }
}

void append_unsigned_le_bytes(std::vector<std::uint8_t>& bytes,
                              std::uint64_t value,
                              std::size_t size_bytes) {
  for (std::size_t offset = 0; offset < size_bytes; ++offset) {
    bytes.push_back(static_cast<std::uint8_t>((value >> (offset * 8)) & 0xffu));
  }
}

[[nodiscard]] std::optional<std::vector<std::uint8_t>> immediate_value_bytes(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto size_bytes = immediate_value_size_bytes(value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }

  std::uint64_t payload = 0;
  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      payload = static_cast<std::uint64_t>(value.immediate) & 0xffu;
      break;
    case bir::TypeKind::I16:
      payload = static_cast<std::uint64_t>(value.immediate) & 0xffffu;
      break;
    case bir::TypeKind::I32:
      payload = static_cast<std::uint64_t>(value.immediate) & 0xffffffffu;
      break;
    case bir::TypeKind::I64:
      payload = static_cast<std::uint64_t>(value.immediate);
      break;
    case bir::TypeKind::F32:
      payload = value.immediate_bits & 0xffffffffu;
      break;
    case bir::TypeKind::F64:
      payload = value.immediate_bits;
      break;
    default:
      return std::nullopt;
  }

  std::vector<std::uint8_t> bytes;
  bytes.reserve(*size_bytes);
  append_unsigned_le_bytes(bytes, payload, *size_bytes);
  return bytes;
}

[[nodiscard]] std::optional<std::vector<std::uint8_t>> global_initializer_bytes(
    const bir::Global& global) {
  if (global.is_extern || global.is_thread_local ||
      global.initializer_symbol_name.has_value() ||
      global.initializer_symbol_name_id != kInvalidLinkName) {
    return std::nullopt;
  }

  if (global.initializer.has_value() && global.initializer_elements.empty()) {
    if (global.initializer->type != global.type) {
      return std::nullopt;
    }
    auto bytes = immediate_value_bytes(*global.initializer);
    if (!bytes.has_value() || bytes->size() != global.size_bytes) {
      return std::nullopt;
    }
    return bytes;
  }

  if (!global.initializer_elements.empty() && !global.initializer.has_value()) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(global.size_bytes);
    for (const auto& element : global.initializer_elements) {
      auto element_bytes = immediate_value_bytes(element);
      if (!element_bytes.has_value()) {
        return std::nullopt;
      }
      bytes.insert(bytes.end(), element_bytes->begin(), element_bytes->end());
    }
    if (bytes.size() != global.size_bytes) {
      return std::nullopt;
    }
    return bytes;
  }

  return std::nullopt;
}

[[nodiscard]] bool bytes_are_all_zero(const std::vector<std::uint8_t>& bytes) {
  return std::all_of(bytes.begin(), bytes.end(), [](std::uint8_t byte) {
    return byte == 0;
  });
}

[[nodiscard]] PreparedGlobalObjectData unsupported_global_object_data(
    const PreparedBirModule& prepared,
    const bir::Global& global) {
  const std::string label =
      global.link_name_id == kInvalidLinkName
          ? std::string{}
          : std::string{prepared.names.link_names.spelling(global.link_name_id)};
  const std::string module_label =
      global.link_name_id == kInvalidLinkName
          ? std::string{}
          : std::string{prepared.module.names.link_names.spelling(global.link_name_id)};
  return PreparedGlobalObjectData{
      .object_label = global.link_name_id == kInvalidLinkName
                          ? std::optional<LinkNameId>{}
                          : std::optional<LinkNameId>{global.link_name_id},
      .object_label_text = !label.empty() ? label : module_label,
      .object_size_bytes = global.size_bytes,
      .align_bytes = global.align_bytes,
      .has_object_label = global.link_name_id != kInvalidLinkName,
      .has_publication_identity = global.align_bytes != 0,
      .has_object_byte_range = global.size_bytes != 0,
      .requires_unsupported_marker = true,
      .has_unsupported_marker = true,
      .unsupported_but_coherent = true,
  };
}

}  // namespace

const PreparedGlobalObjectData* find_prepared_global_object_data(
    const PreparedObjectDataPlans& plans,
    LinkNameId object_label) {
  for (const auto& global : plans.globals) {
    if (global.object_label == object_label) {
      return &global;
    }
  }
  return nullptr;
}

void populate_prepared_object_data_plans(PreparedBirModule& prepared) {
  prepared.object_data.globals.clear();
  for (const auto& global : prepared.module.globals) {
    if (global.is_extern && !global.initializer.has_value() &&
        !global.initializer_symbol_name.has_value() &&
        global.initializer_symbol_name_id == kInvalidLinkName &&
        global.initializer_elements.empty()) {
      continue;
    }

    const auto bytes = global_initializer_bytes(global);
    if (!bytes.has_value() || global.link_name_id == kInvalidLinkName ||
        global.align_bytes == 0 || global.size_bytes == 0 ||
        global.is_thread_local ||
        global.address_materialization_policy ==
            bir::GlobalAddressMaterializationPolicy::GotRequired) {
      prepared.object_data.globals.push_back(
          unsupported_global_object_data(prepared, global));
      continue;
    }

    std::string label{prepared.names.link_names.spelling(global.link_name_id)};
    if (label.empty()) {
      label = std::string{
          prepared.module.names.link_names.spelling(global.link_name_id)};
    }

    const bool zero_fill = !global.is_constant && bytes_are_all_zero(*bytes);
    prepared.object_data.globals.push_back(PreparedGlobalObjectData{
        .object_label = global.link_name_id,
        .object_label_text = std::move(label),
        .section_kind = zero_fill ? PreparedObjectDataSectionKind::Bss
                                  : (global.is_constant
                                         ? PreparedObjectDataSectionKind::ReadOnlyData
                                         : PreparedObjectDataSectionKind::Data),
        .object_byte_offset = 0,
        .object_size_bytes = bytes->size(),
        .align_bytes = global.align_bytes,
        .emitted_bytes = zero_fill ? std::vector<std::uint8_t>{} : *bytes,
        .zero_fill_byte_count = zero_fill ? bytes->size() : 0,
        .public_symbol = true,
        .has_object_label = true,
        .has_publication_identity = true,
        .has_object_byte_range = true,
        .requires_emitted_bytes = !zero_fill,
        .has_emitted_bytes = !zero_fill,
        .requires_zero_fill = zero_fill,
        .has_zero_fill = zero_fill,
    });
  }
}

}  // namespace c4c::backend::prepare
