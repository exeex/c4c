#pragma once

#include "names.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;

enum class PreparedObjectDataSectionKind {
  Data,
  ReadOnlyData,
  Bss,
};

struct PreparedGlobalObjectData {
  std::optional<LinkNameId> object_label;
  std::string object_label_text;
  PreparedObjectDataSectionKind section_kind = PreparedObjectDataSectionKind::Data;
  std::size_t object_byte_offset = 0;
  std::size_t object_size_bytes = 0;
  std::size_t align_bytes = 0;
  std::vector<std::uint8_t> emitted_bytes;
  std::size_t zero_fill_byte_count = 0;
  bool public_symbol = true;
  bool has_object_label = false;
  bool has_publication_identity = false;
  bool has_object_byte_range = false;
  bool requires_emitted_bytes = false;
  bool has_emitted_bytes = false;
  bool requires_zero_fill = false;
  bool has_zero_fill = false;
  bool requires_relocation = false;
  bool has_relocation = false;
  bool requires_unsupported_marker = false;
  bool has_unsupported_marker = false;
  bool conflicting_object_label = false;
  bool conflicting_publication_identity = false;
  bool conflicting_emitted_bytes = false;
  bool conflicting_zero_fill = false;
  bool conflicting_relocation = false;
  bool conflicting_object_byte_range = false;
  bool conflicting_unsupported_marker = false;
  bool unsupported_but_coherent = false;
  bool invalid_pre_prepared_initializer_semantics = false;
};

struct PreparedObjectDataPlans {
  std::vector<PreparedGlobalObjectData> globals;
};

[[nodiscard]] const PreparedGlobalObjectData* find_prepared_global_object_data(
    const PreparedObjectDataPlans& plans,
    LinkNameId object_label);

void populate_prepared_object_data_plans(PreparedBirModule& prepared);

}  // namespace c4c::backend::prepare
