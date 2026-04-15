#pragma once

#include "../bir.hpp"
#include "../../codegen/lir/ir.hpp"
#include "../target.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PrepareOptions {
  bool run_legalize = true;
  bool run_stack_layout = true;
  bool run_liveness = true;
  bool run_regalloc = true;
};

struct PrepareNote {
  std::string phase;
  std::string message;
};

struct PreparedStackObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedStackLayout {
  std::vector<PreparedStackObject> objects;
};

struct PreparedLivenessObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  std::string contract_kind;
  std::string access_shape;
  std::string first_access_kind;
  std::string last_access_kind;
  std::size_t direct_read_count = 0;
  std::size_t direct_write_count = 0;
  std::size_t addressed_access_count = 0;
  std::size_t call_arg_exposure_count = 0;
  bool has_access_window = false;
  std::size_t first_access_instruction_index = 0;
  std::size_t last_access_instruction_index = 0;
  bool crosses_call_boundary = false;
};

struct PreparedLivenessFunction {
  std::string function_name;
  std::size_t instruction_count = 0;
  std::size_t call_instruction_count = 0;
  std::size_t object_count = 0;
  std::vector<std::size_t> call_instruction_indices;
};

struct PreparedLiveness {
  std::vector<PreparedLivenessFunction> functions;
  std::vector<PreparedLivenessObject> objects;
};

struct PreparedRegallocObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  std::string contract_kind;
  std::string allocation_kind;
  std::string priority_bucket;
  std::string spill_restore_locality_hint;
  std::string spill_sync_hint;
  std::string home_slot_stability_hint;
  std::string allocation_state_kind;
  std::string reservation_kind;
  std::string reservation_scope;
  std::string home_slot_mode;
  std::string sync_policy;
  std::string follow_up_category;
  std::string sync_coordination_category;
  std::string home_slot_category;
  std::string window_coordination_category;
  std::string binding_frontier_kind;
  std::string binding_frontier_reason;
  std::string binding_batch_kind;
  std::size_t binding_order_index = 0;
  std::string deferred_reason;
  std::string access_shape;
  std::string first_access_kind;
  std::string last_access_kind;
  std::size_t direct_read_count = 0;
  std::size_t direct_write_count = 0;
  std::size_t addressed_access_count = 0;
  std::size_t call_arg_exposure_count = 0;
  bool has_access_window = false;
  std::size_t first_access_instruction_index = 0;
  std::size_t last_access_instruction_index = 0;
  bool crosses_call_boundary = false;
};

struct PreparedRegallocAllocationDecision {
  std::string source_kind;
  std::string source_name;
  std::string allocation_stage;
  std::string reservation_kind;
  std::string reservation_scope;
  std::string home_slot_mode;
  std::string sync_policy;
};

struct PreparedRegallocReservationSummary {
  std::string allocation_stage;
  std::size_t candidate_count = 0;
  std::size_t overlapping_window_count = 0;
  std::size_t unobserved_window_count = 0;
  std::size_t call_boundary_window_count = 0;
  std::size_t single_instruction_window_count = 0;
  std::size_t adjacent_instruction_window_count = 0;
  std::size_t wide_instruction_window_count = 0;
  std::size_t stable_home_slot_required_count = 0;
  std::size_t stable_home_slot_preferred_count = 0;
  std::size_t single_use_home_slot_ok_count = 0;
  std::size_t restore_before_read_count = 0;
  std::size_t writeback_after_write_count = 0;
  std::size_t sync_on_read_write_boundaries_count = 0;
  std::string pressure_signal;
  std::string collision_signal;
};

struct PreparedRegallocContentionSummary {
  std::string allocation_stage;
  std::string follow_up_category;
  std::string sync_coordination_category;
  std::string home_slot_category;
  std::string window_coordination_category;
};

struct PreparedRegallocBindingDecision {
  std::string source_kind;
  std::string source_name;
  std::string binding_batch_kind;
  std::size_t binding_order_index = 0;
};

struct PreparedRegallocBindingBatchSummary {
  std::string binding_batch_kind;
  std::string allocation_stage;
  std::string follow_up_category;
  std::string ordering_policy;
  std::string home_slot_prerequisite_category;
  std::string home_slot_prerequisite_state;
  std::string sync_handoff_prerequisite_category;
  std::string sync_handoff_state;
  std::size_t candidate_count = 0;
};

struct PreparedRegallocDeferredBindingBatchSummary {
  std::string binding_batch_kind;
  std::string allocation_stage;
  std::string deferred_reason;
  std::string follow_up_category;
  std::string ordering_policy;
  std::string access_window_prerequisite_category;
  std::string access_window_prerequisite_state;
  std::string home_slot_prerequisite_category;
  std::string home_slot_prerequisite_state;
  std::string sync_handoff_prerequisite_category;
  std::string sync_handoff_state;
  std::size_t candidate_count = 0;
};

struct PreparedRegallocBindingHandoffSummary {
  std::string binding_frontier_kind;
  std::string binding_frontier_reason;
  std::string binding_batch_kind;
  std::string allocation_stage;
  std::string follow_up_category;
  std::string ordering_policy;
  std::string access_window_prerequisite_category;
  std::string access_window_prerequisite_state;
  std::string home_slot_prerequisite_category;
  std::string home_slot_prerequisite_state;
  std::string sync_handoff_prerequisite_category;
  std::string sync_handoff_state;
  std::size_t candidate_count = 0;
};

struct PreparedRegallocFunction {
  std::string function_name;
  std::vector<PreparedRegallocObject> objects;
  std::vector<PreparedRegallocAllocationDecision> allocation_sequence;
  std::vector<PreparedRegallocReservationSummary> reservation_summary;
  std::vector<PreparedRegallocContentionSummary> contention_summary;
  std::vector<PreparedRegallocBindingDecision> binding_sequence;
  std::vector<PreparedRegallocBindingBatchSummary> binding_batches;
  std::vector<PreparedRegallocDeferredBindingBatchSummary> deferred_binding_batches;
  std::vector<PreparedRegallocBindingHandoffSummary> binding_handoff_summary;
  std::size_t register_candidate_count = 0;
  std::size_t fixed_stack_storage_count = 0;
  std::size_t binding_ready_count = 0;
  std::size_t binding_deferred_count = 0;
  std::size_t binding_deferred_access_window_count = 0;
  std::size_t binding_deferred_coordination_count = 0;
  std::size_t binding_ready_batch_count = 0;
  std::size_t binding_deferred_batch_count = 0;
};

struct PreparedRegalloc {
  std::vector<PreparedRegallocFunction> functions;
};

enum class PrepareRoute {
  SemanticBirShared,
  BootstrapLirFallback,
};

[[nodiscard]] constexpr std::string_view prepare_route_name(PrepareRoute route) {
  switch (route) {
    case PrepareRoute::SemanticBirShared:
      return "semantic_bir_shared";
    case PrepareRoute::BootstrapLirFallback:
      return "bootstrap_lir_fallback";
  }
  return "unknown";
}

enum class PreparedBirInvariant {
  NoTargetFacingI1,
  NoPhiNodes,
};

[[nodiscard]] constexpr std::string_view prepared_bir_invariant_name(PreparedBirInvariant invariant) {
  switch (invariant) {
    case PreparedBirInvariant::NoTargetFacingI1:
      return "no_target_facing_i1";
    case PreparedBirInvariant::NoPhiNodes:
      return "no_phi_nodes";
  }
  return "unknown";
}

struct PreparedLirModule {
  c4c::codegen::lir::LirModule module;
  Target target = Target::X86_64;
  PrepareRoute route = PrepareRoute::BootstrapLirFallback;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

struct PreparedBirModule {
  c4c::backend::bir::Module module;
  Target target = Target::X86_64;
  PrepareRoute route = PrepareRoute::SemanticBirShared;
  std::vector<PreparedBirInvariant> invariants;
  PreparedStackLayout stack_layout;
  PreparedLiveness liveness;
  PreparedRegalloc regalloc;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

PreparedLirModule prepare_bootstrap_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options = {});

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

PreparedLirModule prepare_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options = {});

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare
