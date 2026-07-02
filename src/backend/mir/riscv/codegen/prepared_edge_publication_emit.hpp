#pragma once

#include "emit.hpp"

#include <string_view>

namespace c4c::backend::riscv::codegen {

// Edge-publication move adaptation keeps its public API in emit.hpp for
// compatibility with existing tests and callers.

[[nodiscard]] bool prepared_select_publication_move_is_rv64_object_admitted(
    const EdgePublicationMoveIntent& intent);

[[nodiscard]] bool
prepared_select_publication_pointer_stack_source_to_gpr_is_admitted(
    const EdgePublicationMoveIntent& intent);

[[nodiscard]] bool
prepared_select_publication_gpr_to_stack_destination_is_admitted(
    const EdgePublicationMoveIntent& intent);

[[nodiscard]] std::string_view edge_publication_move_intent_status_name(
    EdgePublicationMoveIntentStatus status);

[[nodiscard]] std::string_view prepared_edge_publication_lookup_status_name(
    c4c::backend::prepare::PreparedEdgePublicationLookupStatus status);

[[nodiscard]] std::string rv64_select_publication_move_rejection_reason(
    const EdgePublicationMoveIntent& intent);

[[nodiscard]] bool
prepared_select_publication_pointer_stack_source_to_gpr_matches_bundle(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedParallelCopyBundle& bundle);

[[nodiscard]] bool
prepared_select_publication_gpr_to_stack_destination_matches_bundle(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedParallelCopyBundle& bundle);

}  // namespace c4c::backend::riscv::codegen
