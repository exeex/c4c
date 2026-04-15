#include "prealloc.hpp"

#include <algorithm>
#include <optional>

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/regalloc.rs for the intended
// register-allocation pipeline and policy shape.

namespace c4c::backend::prepare {

namespace {

std::string_view regalloc_allocation_kind(std::string_view contract_kind) {
  if (contract_kind == "address_exposed_storage") {
    return "fixed_stack_storage";
  }
  return "register_candidate";
}

std::string_view regalloc_access_shape_suffix(std::string_view access_shape) {
  if (access_shape == "direct_read_only") {
    return "read";
  }
  if (access_shape == "direct_write_only") {
    return "write";
  }
  if (access_shape == "direct_read_write") {
    return "read_write";
  }
  if (access_shape == "addressed_access_only") {
    return "addressed";
  }
  if (access_shape == "call_argument_exposure_only") {
    return "call_exposed";
  }
  if (access_shape == "no_access") {
    return "no_access";
  }
  return "mixed_access";
}

bool string_view_starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

std::string_view regalloc_priority_bucket(std::string_view contract_kind,
                                          const PreparedLivenessObject& object) {
  if (contract_kind != "value_storage") {
    return "non_value_storage";
  }
  if (!object.has_access_window ||
      object.first_access_instruction_index == object.last_access_instruction_index) {
    return "single_point_value";
  }
  if (object.crosses_call_boundary) {
    return "call_spanning_value";
  }
  return "multi_point_value";
}

std::string_view regalloc_spill_restore_locality_hint(
    std::string_view allocation_kind,
    const PreparedLivenessObject& object) {
  if (allocation_kind != "register_candidate") {
    if (object.addressed_access_count != 0) {
      return "fixed_stack_address_anchor";
    }
    if (object.call_arg_exposure_count != 0) {
      return "fixed_stack_call_boundary_anchor";
    }
    return "fixed_stack_only";
  }

  if (!object.has_access_window) {
    return "unobserved_access_window";
  }
  if (object.crosses_call_boundary) {
    return "call_split_reuse_window";
  }

  const auto access_window_width =
      object.last_access_instruction_index - object.first_access_instruction_index;
  if (access_window_width == 0) {
    return "single_instruction_reuse_window";
  }
  if (access_window_width == 1) {
    return "adjacent_instruction_reuse_window";
  }
  return "wide_reuse_window";
}

std::string_view regalloc_spill_sync_hint(std::string_view allocation_kind,
                                          const PreparedLivenessObject& object) {
  if (allocation_kind != "register_candidate") {
    if (object.addressed_access_count != 0) {
      return "fixed_stack_memory_authoritative";
    }
    if (object.call_arg_exposure_count != 0) {
      return "fixed_stack_call_boundary_authoritative";
    }
    return "fixed_stack_only";
  }

  if (!object.has_access_window) {
    return "spill_sync_unobserved";
  }

  if (object.direct_write_count == 0) {
    if (object.crosses_call_boundary) {
      return "restore_only_call_window";
    }
    if (object.direct_read_count <= 1) {
      return "restore_only_single_use";
    }
    return "restore_only_reuse_window";
  }

  if (object.direct_read_count == 0) {
    if (object.crosses_call_boundary) {
      return "writeback_only_call_window";
    }
    if (object.direct_write_count <= 1) {
      return "writeback_only_single_definition";
    }
    return "writeback_only_redefinition_window";
  }

  if (object.crosses_call_boundary) {
    return "bidirectional_sync_call_window";
  }
  return "bidirectional_sync_local_window";
}

std::string_view regalloc_home_slot_stability_hint(
    std::string_view allocation_kind,
    const PreparedLivenessObject& object) {
  if (allocation_kind != "register_candidate") {
    if (object.addressed_access_count != 0) {
      return "memory_anchor_home_slot";
    }
    if (object.call_arg_exposure_count != 0) {
      return "call_boundary_anchor_home_slot";
    }
    return "fixed_stack_only";
  }

  if (!object.has_access_window) {
    return "home_slot_unobserved";
  }

  if (object.crosses_call_boundary) {
    if (object.direct_write_count == 0) {
      return "call_preserved_read_home_slot";
    }
    if (object.direct_read_count == 0) {
      return "call_preserved_write_home_slot";
    }
    return "call_preserved_read_write_home_slot";
  }

  const auto access_window_width =
      object.last_access_instruction_index - object.first_access_instruction_index;
  if (object.direct_write_count == 0) {
    if (access_window_width == 0) {
      return "single_use_read_home_slot";
    }
    if (access_window_width == 1) {
      return "adjacent_read_home_slot";
    }
    return "wide_read_home_slot";
  }

  if (object.direct_read_count == 0) {
    if (access_window_width == 0) {
      return "single_definition_write_home_slot";
    }
    if (access_window_width == 1) {
      return "adjacent_write_home_slot";
    }
    return "wide_write_home_slot";
  }

  if (access_window_width <= 1) {
    return "tight_read_write_home_slot";
  }
  return "wide_read_write_home_slot";
}

std::string_view regalloc_allocation_stage(const PreparedRegallocObject& object) {
  if (object.allocation_kind != "register_candidate") {
    return "fixed_stack_storage";
  }
  if (object.priority_bucket == "call_spanning_value") {
    return "stabilize_across_calls";
  }
  if (object.priority_bucket == "multi_point_value") {
    return "stabilize_local_reuse";
  }
  return "opportunistic_single_point";
}

int regalloc_allocation_stage_rank(std::string_view allocation_stage) {
  if (allocation_stage == "stabilize_across_calls") {
    return 0;
  }
  if (allocation_stage == "stabilize_local_reuse") {
    return 1;
  }
  if (allocation_stage == "opportunistic_single_point") {
    return 2;
  }
  return 3;
}

int regalloc_access_shape_rank(const PreparedRegallocObject& object) {
  if (object.access_shape == "direct_read_write") {
    return 0;
  }
  if (object.access_shape == "direct_read_only") {
    return 1;
  }
  if (object.access_shape == "direct_write_only") {
    return 2;
  }
  if (object.access_shape == "addressed_access_only") {
    return 3;
  }
  if (object.access_shape == "call_argument_exposure_only") {
    return 4;
  }
  return 5;
}

std::string_view regalloc_reservation_kind(const PreparedRegallocObject& object) {
  const auto allocation_stage = regalloc_allocation_stage(object);
  if (allocation_stage == "stabilize_across_calls") {
    if (object.spill_sync_hint == "restore_only_call_window") {
      return "call_preserved_read_cache";
    }
    if (object.spill_sync_hint == "writeback_only_call_window") {
      return "call_preserved_writeback_buffer";
    }
    return "call_preserved_value_reservation";
  }
  if (allocation_stage == "stabilize_local_reuse") {
    if (object.access_shape == "direct_read_only") {
      return "local_read_cache_reservation";
    }
    if (object.access_shape == "direct_write_only") {
      return "local_write_buffer_reservation";
    }
    return "local_reuse_value_reservation";
  }
  if (object.access_shape == "direct_read_only") {
    return "single_read_cache_opportunity";
  }
  if (object.access_shape == "direct_write_only") {
    return "single_write_buffer_opportunity";
  }
  return "single_point_value_opportunity";
}

std::string_view regalloc_reservation_scope(const PreparedRegallocObject& object) {
  if (!object.has_access_window) {
    return "unobserved_instruction_window";
  }
  if (object.crosses_call_boundary) {
    return "call_boundary_window";
  }

  const auto access_window_width =
      object.last_access_instruction_index - object.first_access_instruction_index;
  if (access_window_width == 0) {
    return "single_instruction_window";
  }
  if (access_window_width == 1) {
    return "adjacent_instruction_window";
  }
  return "wide_instruction_window";
}

std::string_view regalloc_home_slot_mode(std::string_view home_slot_stability_hint) {
  if (home_slot_stability_hint == "home_slot_unobserved") {
    return "home_slot_needs_future_analysis";
  }
  if (home_slot_stability_hint == "memory_anchor_home_slot" ||
      home_slot_stability_hint == "call_boundary_anchor_home_slot" ||
      string_view_starts_with(home_slot_stability_hint, "call_preserved_")) {
    return "stable_home_slot_required";
  }
  if (home_slot_stability_hint == "single_use_read_home_slot" ||
      home_slot_stability_hint == "single_definition_write_home_slot") {
    return "single_use_home_slot_ok";
  }
  return "stable_home_slot_preferred";
}

std::string_view regalloc_sync_policy(std::string_view spill_sync_hint) {
  if (string_view_starts_with(spill_sync_hint, "restore_only_")) {
    return "restore_before_read";
  }
  if (string_view_starts_with(spill_sync_hint, "writeback_only_")) {
    return "writeback_after_write";
  }
  if (string_view_starts_with(spill_sync_hint, "bidirectional_sync_")) {
    return "sync_on_read_write_boundaries";
  }
  return "sync_policy_needs_future_analysis";
}

bool regalloc_binding_frontier_is_incomplete(const PreparedRegallocObject& object) {
  return object.allocation_kind == "register_candidate" &&
         object.allocation_state_kind == "allocation_state_incomplete";
}

bool regalloc_binding_frontier_is_deferred(const PreparedRegallocObject& object) {
  if (object.allocation_kind != "register_candidate" ||
      regalloc_binding_frontier_is_incomplete(object)) {
    return false;
  }
  if (object.reservation_scope == "unobserved_instruction_window" ||
      object.home_slot_mode == "home_slot_needs_future_analysis" ||
      object.sync_policy == "sync_policy_needs_future_analysis") {
    return true;
  }
  return object.allocation_state_kind == "deferred_single_point_candidate" ||
         object.allocation_state_kind == "opportunistic_single_point_candidate";
}

bool regalloc_binding_frontier_is_ready(const PreparedRegallocObject& object) {
  return object.allocation_kind == "register_candidate" &&
         !regalloc_binding_frontier_is_incomplete(object) &&
         !regalloc_binding_frontier_is_deferred(object);
}

bool access_windows_overlap(const PreparedRegallocObject& lhs, const PreparedRegallocObject& rhs) {
  if (!lhs.has_access_window || !rhs.has_access_window) {
    return false;
  }
  return lhs.first_access_instruction_index <= rhs.last_access_instruction_index &&
         rhs.first_access_instruction_index <= lhs.last_access_instruction_index;
}

std::string regalloc_pressure_signal(const PreparedRegallocReservationSummary& summary) {
  if (summary.candidate_count == 0) {
    return "idle_bucket";
  }
  if (summary.allocation_stage == "stabilize_across_calls") {
    if (summary.overlapping_window_count != 0 &&
        summary.stable_home_slot_required_count == summary.candidate_count) {
      return "call_boundary_stable_home_pressure";
    }
    return "call_boundary_pressure";
  }
  if (summary.allocation_stage == "stabilize_local_reuse") {
    if (summary.overlapping_window_count != 0) {
      return "overlapping_local_reuse_pressure";
    }
    return "sequenced_local_reuse_pressure";
  }
  if (summary.single_instruction_window_count >= 2) {
    return "batched_single_point_pressure";
  }
  if (summary.unobserved_window_count >= 2) {
    return "broad_opportunistic_pressure";
  }
  return "isolated_single_point_pressure";
}

std::string regalloc_collision_signal(const PreparedRegallocReservationSummary& summary) {
  if (summary.overlapping_window_count == 0) {
    if (summary.allocation_stage == "stabilize_local_reuse" && summary.candidate_count > 1) {
      return "sequenced_local_reuse_no_collision";
    }
    if (summary.allocation_stage == "opportunistic_single_point" &&
        summary.single_instruction_window_count >= 2) {
      return "single_instruction_collision_watch";
    }
    return "no_collision_signal";
  }
  if (summary.restore_before_read_count != 0 && summary.writeback_after_write_count != 0 &&
      summary.sync_on_read_write_boundaries_count != 0) {
    return "mixed_sync_window_collision_signal";
  }
  if (summary.restore_before_read_count != 0 && summary.writeback_after_write_count != 0) {
    return "read_write_window_collision_signal";
  }
  if (summary.sync_on_read_write_boundaries_count != 0) {
    return "bidirectional_window_collision_signal";
  }
  return "window_overlap_collision_signal";
}

std::string_view regalloc_follow_up_category(const PreparedRegallocReservationSummary& summary) {
  if (summary.candidate_count == 0) {
    return "idle_follow_up";
  }
  if (summary.allocation_stage == "stabilize_across_calls") {
    return "call_boundary_preservation";
  }
  if (summary.allocation_stage == "stabilize_local_reuse") {
    if (summary.overlapping_window_count != 0) {
      return "overlapping_local_reuse_coordination";
    }
    return "sequenced_local_reuse_coordination";
  }
  if (summary.single_instruction_window_count >= 2) {
    return "batched_single_point_coordination";
  }
  if (summary.unobserved_window_count != 0) {
    return "deferred_single_point_screen";
  }
  return "isolated_single_point_coordination";
}

std::string_view regalloc_sync_coordination_category(
    const PreparedRegallocReservationSummary& summary) {
  const bool has_restore = summary.restore_before_read_count != 0;
  const bool has_writeback = summary.writeback_after_write_count != 0;
  const bool has_bidirectional = summary.sync_on_read_write_boundaries_count != 0;
  if (has_restore && has_writeback && has_bidirectional) {
    return "mixed_sync_coordination";
  }
  if (has_restore && has_writeback) {
    return "read_write_coordination";
  }
  if (has_bidirectional) {
    return "bidirectional_sync_coordination";
  }
  if (has_restore) {
    return "restore_only_coordination";
  }
  if (has_writeback) {
    return "writeback_only_coordination";
  }
  return "sync_free_coordination";
}

std::string_view regalloc_home_slot_category(const PreparedRegallocReservationSummary& summary) {
  if (summary.candidate_count == 0) {
    return "idle_home_slot_coordination";
  }
  if (summary.stable_home_slot_required_count == summary.candidate_count) {
    return "stable_home_slot_required";
  }
  if (summary.stable_home_slot_preferred_count == summary.candidate_count) {
    return "stable_home_slot_preferred";
  }
  if (summary.single_use_home_slot_ok_count == summary.candidate_count) {
    return "single_use_home_slot_ok";
  }
  return "mixed_home_slot_modes";
}

std::string_view regalloc_window_coordination_category(
    const PreparedRegallocReservationSummary& summary) {
  if (summary.candidate_count == 0) {
    return "idle_windows";
  }
  if (summary.overlapping_window_count != 0) {
    if (summary.call_boundary_window_count == summary.candidate_count) {
      return "overlapping_call_boundary_windows";
    }
    return "overlapping_windows";
  }
  if (summary.adjacent_instruction_window_count == summary.candidate_count) {
    return "adjacent_local_windows";
  }
  if (summary.single_instruction_window_count == summary.candidate_count) {
    return "single_instruction_windows";
  }
  if (summary.unobserved_window_count == summary.candidate_count) {
    return "unobserved_windows";
  }
  if (summary.unobserved_window_count != 0 && summary.single_instruction_window_count != 0) {
    return "mixed_sparse_windows";
  }
  if (summary.wide_instruction_window_count != 0) {
    return "wide_local_windows";
  }
  return "sequenced_local_windows";
}

PreparedRegallocContentionSummary summarize_contention_stage(
    const PreparedRegallocReservationSummary& summary) {
  return PreparedRegallocContentionSummary{
      .allocation_stage = summary.allocation_stage,
      .follow_up_category = std::string(regalloc_follow_up_category(summary)),
      .sync_coordination_category = std::string(regalloc_sync_coordination_category(summary)),
      .home_slot_category = std::string(regalloc_home_slot_category(summary)),
      .window_coordination_category = std::string(regalloc_window_coordination_category(summary)),
  };
}

std::string_view regalloc_fixed_stack_reservation_scope(const PreparedRegallocObject& object) {
  if (object.spill_restore_locality_hint == "fixed_stack_address_anchor") {
    return "fixed_stack_memory_anchor";
  }
  if (object.spill_restore_locality_hint == "fixed_stack_call_boundary_anchor") {
    return "fixed_stack_call_boundary_anchor";
  }
  return "fixed_stack_storage";
}

std::string_view regalloc_fixed_stack_sync_policy(const PreparedRegallocObject& object) {
  if (object.spill_sync_hint == "fixed_stack_memory_authoritative") {
    return "memory_authoritative";
  }
  if (object.spill_sync_hint == "fixed_stack_call_boundary_authoritative") {
    return "call_boundary_authoritative";
  }
  return "fixed_stack_only";
}

std::string_view regalloc_allocation_state_kind(
    const PreparedRegallocObject& object,
    const PreparedRegallocAllocationDecision* decision) {
  if (object.allocation_kind != "register_candidate") {
    return "fixed_stack_authoritative";
  }
  if (decision == nullptr) {
    return "allocation_state_incomplete";
  }
  if (decision->allocation_stage == "stabilize_across_calls") {
    return "reserved_call_preserved_candidate";
  }
  if (decision->allocation_stage == "stabilize_local_reuse") {
    return "reserved_local_reuse_candidate";
  }
  if (!object.has_access_window) {
    return "deferred_single_point_candidate";
  }
  return "opportunistic_single_point_candidate";
}

std::string_view regalloc_allocation_state_deferred_reason(
    const PreparedRegallocObject& object,
    const PreparedRegallocAllocationDecision* decision) {
  if (object.allocation_kind != "register_candidate") {
    return "not_applicable_fixed_stack";
  }
  if (decision == nullptr) {
    return "allocation_state_missing_decision";
  }
  if (decision->allocation_stage == "opportunistic_single_point" && !object.has_access_window) {
    return "awaiting_access_window_observation";
  }
  return "not_deferred";
}

std::string_view regalloc_binding_frontier_kind(
    const PreparedRegallocObject& object,
    const PreparedRegallocAllocationDecision* decision,
    const PreparedRegallocContentionSummary* contention) {
  if (object.allocation_kind != "register_candidate") {
    return "fixed_stack_authoritative";
  }
  if (decision == nullptr || contention == nullptr) {
    return "binding_frontier_incomplete";
  }
  if (decision->reservation_scope == "unobserved_instruction_window" ||
      decision->home_slot_mode == "home_slot_needs_future_analysis" ||
      decision->sync_policy == "sync_policy_needs_future_analysis") {
    return "binding_deferred";
  }
  if (decision->allocation_stage == "opportunistic_single_point") {
    return "binding_deferred";
  }
  return "binding_ready";
}

std::string_view regalloc_binding_batch_kind(
    const PreparedRegallocAllocationDecision& decision,
    const PreparedRegallocContentionSummary& contention) {
  (void)decision;
  if (contention.follow_up_category == "call_boundary_preservation") {
    return "call_boundary_binding_batch";
  }
  if (contention.follow_up_category == "sequenced_local_reuse_coordination") {
    return "local_reuse_binding_batch";
  }
  if (contention.follow_up_category == "batched_single_point_coordination") {
    return "deferred_single_point_batch";
  }
  return "binding_batch_needs_future_analysis";
}

std::string_view regalloc_deferred_binding_batch_reason(
    const PreparedRegallocObject& object,
    const PreparedRegallocContentionSummary& contention) {
  if (object.deferred_reason == "awaiting_access_window_observation") {
    return object.deferred_reason;
  }
  if (contention.follow_up_category == "batched_single_point_coordination") {
    return contention.follow_up_category;
  }
  return "binding_deferred_reason_needs_future_analysis";
}

std::string_view regalloc_binding_home_slot_prerequisite_state(
    const PreparedRegallocContentionSummary& contention) {
  if (contention.home_slot_category == "stable_home_slot_required" ||
      contention.home_slot_category == "stable_home_slot_preferred" ||
      contention.home_slot_category == "single_use_home_slot_ok") {
    return "prealloc_home_slot_prerequisite_satisfied";
  }
  if (contention.home_slot_category == "idle_home_slot_coordination") {
    return "no_home_slot_prerequisite";
  }
  return "prealloc_home_slot_prerequisite_deferred";
}

PreparedRegallocDeferredBindingBatchSummary& ensure_deferred_binding_batch_summary(
    PreparedRegallocFunction& function,
    std::string_view deferred_reason) {
  for (auto& summary : function.deferred_binding_batches) {
    if (summary.deferred_reason == deferred_reason) {
      return summary;
    }
  }
  function.deferred_binding_batches.push_back(PreparedRegallocDeferredBindingBatchSummary{
      .deferred_reason = std::string(deferred_reason),
  });
  return function.deferred_binding_batches.back();
}

PreparedRegallocDeferredBindingAttachment regalloc_deferred_binding_attachment(
    const PreparedRegallocFunction& function,
    const PreparedRegallocObject& object) {
  return PreparedRegallocDeferredBindingAttachment{
      .object_index = static_cast<std::size_t>(&object - function.objects.data()),
  };
}

}  // namespace

const PreparedRegallocObject* BirPreAlloc::find_regalloc_object(
    std::string_view source_kind,
    std::string_view source_name) const {
  if (current_regalloc_function_ == nullptr) {
    return nullptr;
  }
  for (const auto& object : current_regalloc_function_->objects) {
    if (object.source_kind == source_kind && object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

PreparedRegallocObject* BirPreAlloc::find_mutable_regalloc_object(
    std::string_view source_kind,
    std::string_view source_name) {
  if (current_regalloc_function_ == nullptr) {
    return nullptr;
  }
  for (auto& object : current_regalloc_function_->objects) {
    if (object.source_kind == source_kind && object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

const PreparedRegallocAllocationDecision* BirPreAlloc::find_allocation_decision(
    std::string_view source_kind,
    std::string_view source_name) const {
  if (current_regalloc_function_ == nullptr) {
    return nullptr;
  }
  for (const auto& decision : current_regalloc_function_->allocation_sequence) {
    if (decision.source_kind == source_kind && decision.source_name == source_name) {
      return &decision;
    }
  }
  return nullptr;
}

const PreparedRegallocContentionSummary* BirPreAlloc::find_contention_summary(
    std::string_view allocation_stage) const {
  if (current_regalloc_function_ == nullptr) {
    return nullptr;
  }
  for (const auto& summary : current_regalloc_function_->contention_summary) {
    if (summary.allocation_stage == allocation_stage) {
      return &summary;
    }
  }
  return nullptr;
}

PreparedRegallocDeferredBindingBatchSummary* BirPreAlloc::find_deferred_binding_batch_summary(
    std::string_view deferred_reason) {
  if (current_regalloc_function_ == nullptr) {
    return nullptr;
  }
  for (auto& summary : current_regalloc_function_->deferred_binding_batches) {
    if (summary.deferred_reason == deferred_reason) {
      return &summary;
    }
  }
  return nullptr;
}

std::size_t regalloc_ready_binding_batch_member_count(
    const PreparedRegallocFunction& function,
    std::string_view binding_batch_kind) {
  return static_cast<std::size_t>(std::count_if(
      function.binding_sequence.begin(), function.binding_sequence.end(),
      [binding_batch_kind](const PreparedRegallocBindingDecision& decision) {
        return decision.binding_batch_kind == binding_batch_kind;
      }));
}

PreparedRegallocReservationSummary BirPreAlloc::summarize_reservation_stage(
    std::string_view allocation_stage) const {
  PreparedRegallocReservationSummary summary{
      .allocation_stage = std::string(allocation_stage),
  };
  if (current_regalloc_function_ == nullptr) {
    return summary;
  }

  std::vector<const PreparedRegallocObject*> stage_objects;
  for (const auto& decision : current_regalloc_function_->allocation_sequence) {
    if (decision.allocation_stage != allocation_stage) {
      continue;
    }
    const auto* object = find_regalloc_object(decision.source_kind, decision.source_name);
    if (object == nullptr) {
      continue;
    }
    stage_objects.push_back(object);
    ++summary.candidate_count;

    if (!object->has_access_window) {
      ++summary.unobserved_window_count;
    } else if (object->crosses_call_boundary) {
      ++summary.call_boundary_window_count;
    } else {
      const auto access_window_width =
          object->last_access_instruction_index - object->first_access_instruction_index;
      if (access_window_width == 0) {
        ++summary.single_instruction_window_count;
      } else if (access_window_width == 1) {
        ++summary.adjacent_instruction_window_count;
      } else {
        ++summary.wide_instruction_window_count;
      }
    }

    if (decision.home_slot_mode == "stable_home_slot_required") {
      ++summary.stable_home_slot_required_count;
    } else if (decision.home_slot_mode == "stable_home_slot_preferred") {
      ++summary.stable_home_slot_preferred_count;
    } else if (decision.home_slot_mode == "single_use_home_slot_ok") {
      ++summary.single_use_home_slot_ok_count;
    }

    if (decision.sync_policy == "restore_before_read") {
      ++summary.restore_before_read_count;
    } else if (decision.sync_policy == "writeback_after_write") {
      ++summary.writeback_after_write_count;
    } else if (decision.sync_policy == "sync_on_read_write_boundaries") {
      ++summary.sync_on_read_write_boundaries_count;
    }
  }

  std::vector<bool> overlaps(stage_objects.size(), false);
  for (std::size_t lhs_index = 0; lhs_index < stage_objects.size(); ++lhs_index) {
    for (std::size_t rhs_index = lhs_index + 1; rhs_index < stage_objects.size(); ++rhs_index) {
      if (!access_windows_overlap(*stage_objects[lhs_index], *stage_objects[rhs_index])) {
        continue;
      }
      overlaps[lhs_index] = true;
      overlaps[rhs_index] = true;
    }
  }
  summary.overlapping_window_count =
      static_cast<std::size_t>(std::count(overlaps.begin(), overlaps.end(), true));
  summary.pressure_signal = regalloc_pressure_signal(summary);
  summary.collision_signal = regalloc_collision_signal(summary);
  return summary;
}

void BirPreAlloc::populate_object_allocation_state() {
  if (current_regalloc_function_ == nullptr) {
    return;
  }
  for (auto& object : current_regalloc_function_->objects) {
    const auto* decision = find_allocation_decision(object.source_kind, object.source_name);
    const auto* contention =
        decision == nullptr ? nullptr : find_contention_summary(decision->allocation_stage);

    object.allocation_state_kind =
        std::string(regalloc_allocation_state_kind(object, decision));
    object.deferred_reason =
        std::string(regalloc_allocation_state_deferred_reason(object, decision));

    if (object.allocation_kind != "register_candidate") {
      object.reservation_kind = "fixed_stack_storage";
      object.reservation_scope = std::string(regalloc_fixed_stack_reservation_scope(object));
      object.home_slot_mode = std::string(regalloc_home_slot_mode(object.home_slot_stability_hint));
      object.sync_policy = std::string(regalloc_fixed_stack_sync_policy(object));
      object.binding_frontier_kind = "fixed_stack_authoritative";
      continue;
    }

    if (decision == nullptr) {
      object.reservation_kind = "allocation_state_missing_decision";
      object.reservation_scope = "allocation_state_missing_decision";
      object.home_slot_mode = "allocation_state_missing_decision";
      object.sync_policy = "allocation_state_missing_decision";
      object.binding_frontier_kind = "binding_frontier_incomplete";
      continue;
    }

    object.reservation_kind = decision->reservation_kind;
    object.reservation_scope = decision->reservation_scope;
    object.home_slot_mode = decision->home_slot_mode;
    object.sync_policy = decision->sync_policy;

    object.binding_frontier_kind =
        std::string(regalloc_binding_frontier_kind(object, decision, contention));
  }
}

void BirPreAlloc::populate_binding_sequence() {
  if (current_regalloc_function_ == nullptr) {
    return;
  }
  current_regalloc_function_->binding_sequence.clear();
  current_regalloc_function_->deferred_binding_batches.clear();

  for (const auto& decision : current_regalloc_function_->allocation_sequence) {
    auto* object = find_mutable_regalloc_object(decision.source_kind, decision.source_name);
    const auto* contention = find_contention_summary(decision.allocation_stage);
    if (object == nullptr || contention == nullptr) {
      continue;
    }

    if (regalloc_binding_frontier_is_deferred(*object)) {
      const std::string deferred_reason(regalloc_deferred_binding_batch_reason(*object, *contention));
      auto& batch_summary =
          ensure_deferred_binding_batch_summary(*current_regalloc_function_, deferred_reason);
      batch_summary.attachments.push_back(
          regalloc_deferred_binding_attachment(*current_regalloc_function_, *object));
      continue;
    }

    if (!regalloc_binding_frontier_is_ready(*object)) {
      continue;
    }

    const std::string binding_batch_kind(regalloc_binding_batch_kind(decision, *contention));
    current_regalloc_function_->binding_sequence.push_back(PreparedRegallocBindingDecision{
        .source_kind = decision.source_kind,
        .source_name = decision.source_name,
        .binding_batch_kind = binding_batch_kind,
        .binding_order_index = regalloc_ready_binding_batch_member_count(
            *current_regalloc_function_, binding_batch_kind),
    });
  }
}

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.module.functions.size());

  for (const auto& function : prepared_.module.functions) {
    PreparedRegallocFunction prepared_function{
        .function_name = function.name,
    };
    current_regalloc_function_ = &prepared_function;
    for (const auto& object : prepared_.liveness.objects) {
      if (object.function_name != function.name) {
        continue;
      }
      const std::string allocation_kind(regalloc_allocation_kind(object.contract_kind));
      const std::string priority_bucket(regalloc_priority_bucket(object.contract_kind, object));
      const std::string spill_restore_locality_hint(
          regalloc_spill_restore_locality_hint(allocation_kind, object));
      const std::string spill_sync_hint(regalloc_spill_sync_hint(allocation_kind, object));
      const std::string home_slot_stability_hint(
          regalloc_home_slot_stability_hint(allocation_kind, object));
      prepared_function.objects.push_back(PreparedRegallocObject{
          .function_name = object.function_name,
          .source_name = object.source_name,
          .source_kind = object.source_kind,
          .contract_kind = object.contract_kind,
          .allocation_kind = allocation_kind,
          .priority_bucket = priority_bucket,
          .spill_restore_locality_hint = spill_restore_locality_hint,
          .spill_sync_hint = spill_sync_hint,
          .home_slot_stability_hint = home_slot_stability_hint,
          .access_shape = object.access_shape,
          .first_access_kind = object.first_access_kind,
          .last_access_kind = object.last_access_kind,
          .direct_read_count = object.direct_read_count,
          .direct_write_count = object.direct_write_count,
          .addressed_access_count = object.addressed_access_count,
          .call_arg_exposure_count = object.call_arg_exposure_count,
          .has_access_window = object.has_access_window,
          .first_access_instruction_index = object.first_access_instruction_index,
          .last_access_instruction_index = object.last_access_instruction_index,
          .crosses_call_boundary = object.crosses_call_boundary,
      });
      if (allocation_kind == "fixed_stack_storage") {
        ++prepared_function.fixed_stack_storage_count;
      } else {
        ++prepared_function.register_candidate_count;
      }
    }
    std::vector<const PreparedRegallocObject*> ordered_register_candidates;
    ordered_register_candidates.reserve(prepared_function.objects.size());
    for (const auto& object : prepared_function.objects) {
      if (object.allocation_kind == "register_candidate") {
        ordered_register_candidates.push_back(&object);
      }
    }
    std::stable_sort(ordered_register_candidates.begin(),
                     ordered_register_candidates.end(),
                     [](const PreparedRegallocObject* lhs, const PreparedRegallocObject* rhs) {
                       const auto lhs_stage = regalloc_allocation_stage(*lhs);
                       const auto rhs_stage = regalloc_allocation_stage(*rhs);
                       if (lhs_stage != rhs_stage) {
                         return regalloc_allocation_stage_rank(lhs_stage) <
                                regalloc_allocation_stage_rank(rhs_stage);
                       }
                       const int lhs_access_shape_rank = regalloc_access_shape_rank(*lhs);
                       const int rhs_access_shape_rank = regalloc_access_shape_rank(*rhs);
                       if (lhs_access_shape_rank != rhs_access_shape_rank) {
                         return lhs_access_shape_rank < rhs_access_shape_rank;
                       }
                       if (lhs->has_access_window != rhs->has_access_window) {
                         return lhs->has_access_window > rhs->has_access_window;
                       }
                       if (lhs->has_access_window && rhs->has_access_window) {
                         const auto lhs_width =
                             lhs->last_access_instruction_index - lhs->first_access_instruction_index;
                         const auto rhs_width =
                             rhs->last_access_instruction_index - rhs->first_access_instruction_index;
                         if (lhs_width != rhs_width) {
                           return lhs_width < rhs_width;
                         }
                       }
                       if (lhs->source_kind != rhs->source_kind) {
                         return lhs->source_kind < rhs->source_kind;
                       }
                       return lhs->source_name < rhs->source_name;
                     });
    prepared_function.allocation_sequence.reserve(ordered_register_candidates.size());
    for (const auto* object : ordered_register_candidates) {
      prepared_function.allocation_sequence.push_back(PreparedRegallocAllocationDecision{
          .source_kind = object->source_kind,
          .source_name = object->source_name,
          .allocation_stage = std::string(regalloc_allocation_stage(*object)),
          .reservation_kind = std::string(regalloc_reservation_kind(*object)),
          .reservation_scope = std::string(regalloc_reservation_scope(*object)),
          .home_slot_mode = std::string(regalloc_home_slot_mode(object->home_slot_stability_hint)),
          .sync_policy = std::string(regalloc_sync_policy(object->spill_sync_hint)),
      });
    }
    constexpr std::string_view kReservationStages[] = {
        "stabilize_across_calls",
        "stabilize_local_reuse",
        "opportunistic_single_point",
    };
    prepared_function.reservation_summary.reserve(std::size(kReservationStages));
    prepared_function.contention_summary.reserve(std::size(kReservationStages));
    for (const auto stage : kReservationStages) {
      auto summary = summarize_reservation_stage(stage);
      if (summary.candidate_count != 0) {
        prepared_function.contention_summary.push_back(summarize_contention_stage(summary));
        prepared_function.reservation_summary.push_back(std::move(summary));
      }
    }
    populate_object_allocation_state();
    populate_binding_sequence();
    if (!prepared_function.objects.empty()) {
      prepared_.regalloc.functions.push_back(std::move(prepared_function));
    }
    current_regalloc_function_ = nullptr;
  }

  prepared_.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message =
          "regalloc now groups prepared liveness objects per function and classifies them as "
          "register_candidate or fixed_stack_storage contracts, plus target-neutral priority "
          "buckets for single-point, multi-point, and call-spanning value-storage objects, "
          "spill/restore-locality hints keyed by access-window width plus fixed-stack "
          "exposure kind, spill-sync hints "
          "keyed by observed read/write direction plus fixed-stack exposure kind, home-slot "
          "stability hints keyed by access-window width and read/write direction plus "
          "fixed-stack exposure kind, "
          "allocation-sequence decisions that consume the current prepared contract into "
          "stabilize_across_calls, stabilize_local_reuse, and opportunistic_single_point "
          "staging for register candidates, plus first-pass reservation decisions that "
          "turn those stages into target-neutral call-preserved, local-reuse, or "
          "single-point register attempts with scope, home-slot, and sync policies, "
          "per-function reservation pressure/collision summaries derived from that "
          "allocation sequence plus the current prepared access-window, sync-policy, "
          "and home-slot facts without naming physical registers or inventing "
          "interference graphs, plus explicit contention follow-up categories that "
          "group those summaries into window, sync, and home-slot coordination "
          "buckets for downstream prepared-BIR consumers, plus concrete object-level "
          "allocation states that collapse staged reservation, coordination, and "
          "deferred-single-point facts back onto each prepared object, plus a "
          "ready-vs-deferred binding frontier that flags which prepared register "
          "candidates are ready for stable home-slot/register binding and which "
          "still wait on access-window observation or coordination buckets, "
          "plus explicit deferred binding batch artifacts that separate "
          "unobserved access-window blockers from coordination-blocked "
          "single-point batches without naming physical registers, plus "
          "per-object binding batch/order membership so downstream prepared "
          "consumers can join objects to batch-owned summaries without "
          "duplicating prerequisite state across every object, plus downstream handoff summaries that collapse "
          "ready, access-window-deferred, and coordination-deferred frontiers "
          "into one scan surface derived from those uniform per-object cues, "
          "plus a ready-only binding batch/order artifact that keeps current "
          "stable-binding work grouped by prepare-owned call-boundary versus "
          "local-reuse batches without naming physical registers, plus per-binding "
          "ordering-policy cues projected from those ready batches so downstream "
          "prepared consumers can read the sequencing contract without consulting "
          "batch summaries alone, plus compact access-shape "
          "summaries, first and last access-kind cues, "
          "direct read/write, addressed-access, and call-argument exposure counts, and "
          "instruction-order access windows and call-crossing cues for downstream prepared-BIR "
          "consumers; physical register assignment remains future work",
  });
}

}  // namespace c4c::backend::prepare
