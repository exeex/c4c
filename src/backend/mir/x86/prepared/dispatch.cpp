#include "prepared.hpp"
#include "../x86.hpp"

#include <string>

namespace c4c::backend::x86::prepared {

namespace {

std::string render_edge_publication_source_operand(
    const c4c::backend::mir::prepared::PreparedMirDirectEdgePublicationSourceView&
        source) {
  if (source.source_stack_offset_bytes.has_value()) {
    return "DWORD PTR [rsp + " + std::to_string(*source.source_stack_offset_bytes) + "]";
  }
  if (source.source_register_name.has_value()) {
    return *source.source_register_name;
  }
  if (source.source_immediate_i32.has_value()) {
    return std::to_string(*source.source_immediate_i32);
  }
  return {};
}

c4c::backend::x86::prepared::EdgePublicationMoveIntentStatus intent_status_from_query_status(
    c4c::backend::mir::prepared::PreparedMirDirectEdgePublicationSourceQueryStatus status) {
  namespace mir_prepared = c4c::backend::mir::prepared;

  switch (status) {
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::Available:
      return EdgePublicationMoveIntentStatus::Available;
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::MissingLookups:
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::
        MissingEdgePublicationLookups:
      return EdgePublicationMoveIntentStatus::MissingSharedLookups;
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::MissingFunctionView:
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::MissingBlock:
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::
        MissingValueLocations:
    case mir_prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::
        MissingSuccessorLabel:
      return EdgePublicationMoveIntentStatus::MissingPublication;
  }
  return EdgePublicationMoveIntentStatus::MissingPublication;
}

}  // namespace

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
    const c4c::backend::mir::prepared::PreparedMirFunctionView& function_view,
    const c4c::backend::x86::ConsumedPlans& consumed,
    std::size_t successor_block_index,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  namespace prepare = c4c::backend::prepare;

  const auto sources =
      function_view.current_block_direct_edge_publication_sources(successor_block_index);
  if (sources.status !=
      c4c::backend::mir::prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::
          Available) {
    return EdgePublicationMoveIntent{
        .status = intent_status_from_query_status(sources.status),
    };
  }

  const c4c::backend::mir::prepared::PreparedMirDirectEdgePublicationSourceView*
      accepted_source = nullptr;
  for (const auto& source : sources.sources) {
    if (source.status == c4c::backend::mir::prepared::
                             PreparedMirDirectEdgePublicationSourceStatus::Available &&
        source.predecessor_label == predecessor_label &&
        source.successor_label == successor_label &&
        source.destination_value_id == destination_value_id) {
      accepted_source = &source;
      break;
    }
  }
  if (accepted_source == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingPublication,
    };
  }

  const auto* lookups = consumed.shared_function_lookups();
  const auto* publication =
      lookups == nullptr
          ? nullptr
          : prepare::find_unique_indexed_prepared_edge_publication(
                &lookups->edge_publications,
                predecessor_label,
                successor_label,
                destination_value_id);
  EdgePublicationMoveIntent intent{
      .status = EdgePublicationMoveIntentStatus::Available,
      .publication = publication,
      .destination_value_id = accepted_source->destination_value_id,
  };
  if (accepted_source->source_value_id.has_value()) {
    intent.source_value_id = *accepted_source->source_value_id;
  }

  if (!accepted_source->destination_register_name.has_value()) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
    return intent;
  }

  const auto source_operand = render_edge_publication_source_operand(*accepted_source);
  if (source_operand.empty()) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }

  intent.source_operand = source_operand;
  intent.destination_operand = *accepted_source->destination_register_name;
  intent.instruction_text = "mov " + intent.destination_operand + ", " + intent.source_operand;
  return intent;
}

EdgePublicationMoveIntent append_edge_publication_move_instruction(
    std::string& output,
    const c4c::backend::mir::prepared::PreparedMirFunctionView& function_view,
    const c4c::backend::x86::ConsumedPlans& consumed,
    std::size_t successor_block_index,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  auto intent = consume_edge_publication_move_intent(
      function_view,
      consumed,
      successor_block_index,
      predecessor_label,
      successor_label,
      destination_value_id);
  if (intent.status == EdgePublicationMoveIntentStatus::Available) {
    output += "    " + intent.instruction_text + "\n";
  }
  return intent;
}

}  // namespace c4c::backend::x86::prepared
