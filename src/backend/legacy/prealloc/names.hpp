#pragma once

#include "../bir/bir.hpp"
#include "../../shared/text_id_table.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::prepare {

using PreparedObjectId = std::size_t;
using PreparedValueId = std::size_t;
using PreparedFrameSlotId = std::size_t;

struct PrepareOptions {
  bool run_legalize = true;
  bool run_stack_layout = true;
  bool run_liveness = true;
  bool run_out_of_ssa = true;
  bool run_regalloc = true;
};

struct PrepareNote {
  std::string phase;
  std::string message;
};

struct PreparedNameTables {
  PreparedNameTables() { reattach(); }

  PreparedNameTables(const PreparedNameTables& other)
      : texts(other.texts),
        function_names(other.function_names),
        block_labels(other.block_labels),
        value_names(other.value_names),
        slot_names(other.slot_names),
        link_names(other.link_names) {
    reattach();
  }

  PreparedNameTables(PreparedNameTables&& other) noexcept
      : texts(std::move(other.texts)),
        function_names(std::move(other.function_names)),
        block_labels(std::move(other.block_labels)),
        value_names(std::move(other.value_names)),
        slot_names(std::move(other.slot_names)),
        link_names(std::move(other.link_names)) {
    reattach();
  }

  PreparedNameTables& operator=(const PreparedNameTables& other) {
    if (this == &other) {
      return *this;
    }
    texts = other.texts;
    function_names = other.function_names;
    block_labels = other.block_labels;
    value_names = other.value_names;
    slot_names = other.slot_names;
    link_names = other.link_names;
    reattach();
    return *this;
  }

  PreparedNameTables& operator=(PreparedNameTables&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    texts = std::move(other.texts);
    function_names = std::move(other.function_names);
    block_labels = std::move(other.block_labels);
    value_names = std::move(other.value_names);
    slot_names = std::move(other.slot_names);
    link_names = std::move(other.link_names);
    reattach();
    return *this;
  }

  TextTable texts;
  FunctionNameTable function_names{&texts};
  BlockLabelTable block_labels{&texts};
  ValueNameTable value_names{&texts};
  SlotNameTable slot_names{&texts};
  LinkNameTable link_names{&texts};

 private:
  void reattach() {
    function_names.attach_text_table(&texts);
    block_labels.attach_text_table(&texts);
    value_names.attach_text_table(&texts);
    slot_names.attach_text_table(&texts);
    link_names.attach_text_table(&texts);
  }
};

struct PreparedBirModule;

[[nodiscard]] inline std::string_view prepared_function_name(
    const PreparedNameTables& names,
    FunctionNameId id) {
  return names.function_names.spelling(id);
}

[[nodiscard]] inline std::string_view prepared_block_label(
    const PreparedNameTables& names,
    BlockLabelId id) {
  return names.block_labels.spelling(id);
}

[[nodiscard]] inline std::string_view prepared_value_name(
    const PreparedNameTables& names,
    ValueNameId id) {
  return names.value_names.spelling(id);
}

[[nodiscard]] inline std::optional<ValueNameId> prepared_named_value_id(
    PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  return names.value_names.intern(value.name);
}

[[nodiscard]] inline std::string_view prepared_slot_name(
    const PreparedNameTables& names,
    SlotNameId id) {
  return names.slot_names.spelling(id);
}

[[nodiscard]] inline SlotNameId intern_prepared_slot_name(
    PreparedNameTables& names,
    const c4c::backend::bir::NameTables& bir_names,
    SlotNameId bir_slot_id,
    std::string_view spelling) {
  if (bir_slot_id != kInvalidSlotName) {
    const std::string_view id_spelling = bir_names.slot_names.spelling(bir_slot_id);
    if (!id_spelling.empty()) {
      return names.slot_names.intern(id_spelling);
    }
  }
  return names.slot_names.intern(spelling);
}

[[nodiscard]] inline std::string_view prepared_link_name(
    const PreparedNameTables& names,
    LinkNameId id) {
  return names.link_names.spelling(id);
}

}  // namespace c4c::backend::prepare
