#include "formal_publications.hpp"

#include "control_flow.hpp"
#include "value_locations.hpp"

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] const PreparedValueHome* find_formal_home(
    const PreparedFormalPublicationInputs& inputs,
    c4c::ValueNameId value_name,
    std::optional<PreparedValueId>& value_id) {
  value_id = find_indexed_prepared_value_id(inputs.value_home_lookups,
                                            nullptr,
                                            inputs.value_locations,
                                            value_name);
  if (!value_id.has_value()) {
    return nullptr;
  }
  return find_indexed_prepared_value_home(inputs.value_home_lookups,
                                          inputs.value_locations,
                                          *value_id);
}

[[nodiscard]] PreparedFormalPublicationPlan base_plan(
    std::size_t formal_index,
    const bir::Param* formal) {
  return PreparedFormalPublicationPlan{
      .formal_index = formal_index,
      .type = formal != nullptr ? formal->type : bir::TypeKind::Void,
      .formal = formal,
  };
}

}  // namespace

bool prepared_formal_publication_available(
    const PreparedFormalPublicationPlan& plan) {
  return plan.status == PreparedFormalPublicationStatus::Available;
}

PreparedFormalPublicationPlan plan_prepared_formal_publication(
    const PreparedFormalPublicationInputs& inputs,
    std::size_t formal_index) {
  if (inputs.names == nullptr || inputs.function == nullptr ||
      inputs.value_locations == nullptr ||
      formal_index >= inputs.function->params.size()) {
    return PreparedFormalPublicationPlan{
        .status = PreparedFormalPublicationStatus::MissingInputs,
        .formal_index = formal_index,
    };
  }

  const auto& formal = inputs.function->params[formal_index];
  auto plan = base_plan(formal_index, &formal);
  if (formal.is_varargs || formal.is_sret) {
    plan.status = PreparedFormalPublicationStatus::NoPublication;
    plan.action = PreparedFormalPublicationAction::NoPublication;
    return plan;
  }

  const auto value_name =
      resolve_prepared_value_name_id(*inputs.names, formal.name);
  if (!value_name.has_value()) {
    plan.status = PreparedFormalPublicationStatus::MissingFormalName;
    return plan;
  }
  plan.value_name = *value_name;

  std::optional<PreparedValueId> value_id;
  const auto* home = find_formal_home(inputs, *value_name, value_id);
  plan.value_id = value_id;
  plan.home = home;
  if (home == nullptr) {
    plan.status = PreparedFormalPublicationStatus::MissingValueHome;
    return plan;
  }
  plan.home_kind = home->kind;

  if (!formal.abi.has_value()) {
    plan.status = PreparedFormalPublicationStatus::MissingAbiInfo;
    return plan;
  }

  if (formal.abi->passed_on_stack) {
    plan.action = PreparedFormalPublicationAction::IncomingStackToHome;
  } else if (formal.abi->passed_in_register) {
    plan.action = PreparedFormalPublicationAction::IncomingRegisterToHome;
  } else {
    plan.status = PreparedFormalPublicationStatus::UnsupportedFormalSource;
    return plan;
  }

  switch (home->kind) {
    case PreparedValueHomeKind::Register:
      if (!home->register_name.has_value()) {
        plan.status = PreparedFormalPublicationStatus::MissingRegisterName;
        return plan;
      }
      break;
    case PreparedValueHomeKind::StackSlot:
      if (!home->offset_bytes.has_value()) {
        plan.status = PreparedFormalPublicationStatus::MissingStackOffset;
        return plan;
      }
      break;
    case PreparedValueHomeKind::None:
      break;
    case PreparedValueHomeKind::RematerializableImmediate:
    case PreparedValueHomeKind::PointerBasePlusOffset:
      plan.status = PreparedFormalPublicationStatus::UnsupportedHomeKind;
      return plan;
  }

  plan.status = PreparedFormalPublicationStatus::Available;
  return plan;
}

std::vector<PreparedFormalPublicationPlan> plan_prepared_formal_publications(
    const PreparedFormalPublicationInputs& inputs) {
  std::vector<PreparedFormalPublicationPlan> plans;
  if (inputs.function == nullptr) {
    return plans;
  }
  plans.reserve(inputs.function->params.size());
  for (std::size_t formal_index = 0;
       formal_index < inputs.function->params.size();
       ++formal_index) {
    plans.push_back(plan_prepared_formal_publication(inputs, formal_index));
  }
  return plans;
}

}  // namespace c4c::backend::prepare
