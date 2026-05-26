#include "prepared.hpp"
#include "../x86.hpp"

#include <string>

namespace c4c::backend::x86::prepared {

FastPath classify_module_fast_path(const c4c::backend::prepare::PreparedBirModule& module,
                                   std::optional<std::string_view> focus_function) {
  FastPath decision{};
  std::size_t defined_functions = 0;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    // `focus_function` is a public debug/entry selector over rendered text;
    // the dispatch lane remains a route-local summary, not symbol authority.
    if (focus_function.has_value() && function.name != *focus_function) {
      continue;
    }
    ++defined_functions;
  }

  decision.accepted = defined_functions <= 1;
  decision.lane = defined_functions <= 1 ? "single-defined-function" : "multi-defined-module";
  decision.reason = decision.accepted
                        ? "bounded contract-first fast path available"
                        : "behavior-recovery packet must reintroduce multi-function dispatch";
  return decision;
}

EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::x86::ConsumedPlans& consumed,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  namespace prepare = c4c::backend::prepare;

  const auto* lookups = consumed.shared_function_lookups();
  if (lookups == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingSharedLookups,
    };
  }

  const auto* publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups->edge_publications, predecessor_label, successor_label,
      destination_value_id);
  if (publication == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingPublication,
    };
  }

  EdgePublicationMoveIntent intent{
      .status = EdgePublicationMoveIntentStatus::UnsupportedPublication,
      .publication = publication,
      .destination_value_id = publication->destination_value_id,
  };
  if (publication->source_value_id.has_value()) {
    intent.source_value_id = *publication->source_value_id;
  }

  if (publication->status != prepare::PreparedEdgePublicationLookupStatus::Available ||
      publication->move == nullptr ||
      publication->move->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return intent;
  }
  if (publication->source_home == nullptr ||
      publication->source_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !publication->source_home->offset_bytes.has_value()) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }
  if (publication->destination_home == nullptr ||
      publication->destination_home->kind != prepare::PreparedValueHomeKind::Register ||
      !publication->destination_home->register_name.has_value()) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
    return intent;
  }

  intent.status = EdgePublicationMoveIntentStatus::Available;
  intent.source_operand =
      "DWORD PTR [rsp + " + std::to_string(*publication->source_home->offset_bytes) + "]";
  intent.destination_operand = *publication->destination_home->register_name;
  intent.instruction_text = "mov " + intent.destination_operand + ", " + intent.source_operand;
  return intent;
}

}  // namespace c4c::backend::x86::prepared
