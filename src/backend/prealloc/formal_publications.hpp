#pragma once

#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedValueHomeLookups;

enum class PreparedFormalPublicationAction {
  NoPublication,
  IncomingRegisterToHome,
  IncomingStackToHome,
};

[[nodiscard]] constexpr std::string_view prepared_formal_publication_action_name(
    PreparedFormalPublicationAction action) {
  switch (action) {
    case PreparedFormalPublicationAction::NoPublication:
      return "no_publication";
    case PreparedFormalPublicationAction::IncomingRegisterToHome:
      return "incoming_register_to_home";
    case PreparedFormalPublicationAction::IncomingStackToHome:
      return "incoming_stack_to_home";
  }
  return "unknown";
}

enum class PreparedFormalPublicationStatus {
  Available,
  NoPublication,
  MissingInputs,
  MissingFormalName,
  MissingValueHome,
  MissingAbiInfo,
  MissingRegisterName,
  MissingStackOffset,
  UnsupportedHomeKind,
  UnsupportedFormalSource,
};

[[nodiscard]] constexpr std::string_view prepared_formal_publication_status_name(
    PreparedFormalPublicationStatus status) {
  switch (status) {
    case PreparedFormalPublicationStatus::Available:
      return "available";
    case PreparedFormalPublicationStatus::NoPublication:
      return "no_publication";
    case PreparedFormalPublicationStatus::MissingInputs:
      return "missing_inputs";
    case PreparedFormalPublicationStatus::MissingFormalName:
      return "missing_formal_name";
    case PreparedFormalPublicationStatus::MissingValueHome:
      return "missing_value_home";
    case PreparedFormalPublicationStatus::MissingAbiInfo:
      return "missing_abi_info";
    case PreparedFormalPublicationStatus::MissingRegisterName:
      return "missing_register_name";
    case PreparedFormalPublicationStatus::MissingStackOffset:
      return "missing_stack_offset";
    case PreparedFormalPublicationStatus::UnsupportedHomeKind:
      return "unsupported_home_kind";
    case PreparedFormalPublicationStatus::UnsupportedFormalSource:
      return "unsupported_formal_source";
  }
  return "unknown";
}

struct PreparedFormalPublicationPlan {
  PreparedFormalPublicationStatus status =
      PreparedFormalPublicationStatus::MissingInputs;
  PreparedFormalPublicationAction action =
      PreparedFormalPublicationAction::NoPublication;
  std::size_t formal_index = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::optional<PreparedValueId> value_id;
  bir::TypeKind type = bir::TypeKind::Void;
  const bir::Param* formal = nullptr;
  const PreparedValueHome* home = nullptr;
  PreparedValueHomeKind home_kind = PreparedValueHomeKind::None;
};

struct PreparedFormalPublicationInputs {
  const PreparedNameTables* names = nullptr;
  const bir::Function* function = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
};

[[nodiscard]] bool prepared_formal_publication_available(
    const PreparedFormalPublicationPlan& plan);

[[nodiscard]] PreparedFormalPublicationPlan plan_prepared_formal_publication(
    const PreparedFormalPublicationInputs& inputs,
    std::size_t formal_index);

[[nodiscard]] std::vector<PreparedFormalPublicationPlan>
plan_prepared_formal_publications(const PreparedFormalPublicationInputs& inputs);

}  // namespace c4c::backend::prepare
