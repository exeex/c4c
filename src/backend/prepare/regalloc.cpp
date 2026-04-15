#include "regalloc.hpp"

#include <algorithm>

// Execution note: this file is still a scaffold.
// Follow ref/claudes-c-compiler/src/backend/regalloc.rs for the intended
// register-allocation pipeline and policy shape.

namespace c4c::backend::prepare {

namespace {

enum class RegallocAccessKind {
  None,
  DirectRead,
  DirectWrite,
  AddressedAccess,
  CallArgumentExposure,
};

struct RegallocObjectAccessSummary {
  std::size_t direct_read_count = 0;
  std::size_t direct_write_count = 0;
  std::size_t addressed_access_count = 0;
  std::size_t call_arg_exposure_count = 0;
  bool has_access_window = false;
  std::size_t first_access_instruction_index = 0;
  std::size_t last_access_instruction_index = 0;
  RegallocAccessKind first_access_kind = RegallocAccessKind::None;
  RegallocAccessKind last_access_kind = RegallocAccessKind::None;
  std::vector<std::size_t> call_instruction_indices;
};

void record_access(RegallocObjectAccessSummary& summary,
                   std::size_t instruction_index,
                   RegallocAccessKind access_kind) {
  if (!summary.has_access_window) {
    summary.has_access_window = true;
    summary.first_access_instruction_index = instruction_index;
    summary.first_access_kind = access_kind;
  }
  summary.last_access_instruction_index = instruction_index;
  summary.last_access_kind = access_kind;
}

std::string_view regalloc_allocation_kind(std::string_view contract_kind) {
  if (contract_kind == "address_exposed_storage") {
    return "fixed_stack_storage";
  }
  return "register_candidate";
}

bool memory_address_targets_object(const std::optional<bir::MemoryAddress>& address,
                                   std::string_view source_name) {
  return address.has_value() && address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
         address->base_name == source_name;
}

bool named_value_targets_object(const bir::Value& value, std::string_view source_name) {
  return value.kind == bir::Value::Kind::Named && value.name == source_name;
}

std::string_view regalloc_access_kind_name(RegallocAccessKind access_kind) {
  switch (access_kind) {
    case RegallocAccessKind::None:
      return "none";
    case RegallocAccessKind::DirectRead:
      return "direct_read";
    case RegallocAccessKind::DirectWrite:
      return "direct_write";
    case RegallocAccessKind::AddressedAccess:
      return "addressed_access";
    case RegallocAccessKind::CallArgumentExposure:
      return "call_argument_exposure";
  }
  return "unknown";
}

std::string_view regalloc_access_shape_name(const RegallocObjectAccessSummary& summary) {
  const bool has_direct_read = summary.direct_read_count != 0;
  const bool has_direct_write = summary.direct_write_count != 0;
  const bool has_addressed_access = summary.addressed_access_count != 0;
  const bool has_call_arg_exposure = summary.call_arg_exposure_count != 0;
  const std::size_t active_category_count = static_cast<std::size_t>(has_direct_read) +
                                            static_cast<std::size_t>(has_direct_write) +
                                            static_cast<std::size_t>(has_addressed_access) +
                                            static_cast<std::size_t>(has_call_arg_exposure);
  if (active_category_count == 0) {
    return "no_access";
  }
  if (active_category_count == 1) {
    if (has_direct_read) {
      return "direct_read_only";
    }
    if (has_direct_write) {
      return "direct_write_only";
    }
    if (has_addressed_access) {
      return "addressed_access_only";
    }
    return "call_argument_exposure_only";
  }
  if (active_category_count == 2 && has_direct_read && has_direct_write &&
      !has_addressed_access && !has_call_arg_exposure) {
    return "direct_read_write";
  }
  return "mixed_access_shape";
}

std::string_view regalloc_access_shape_suffix(const RegallocObjectAccessSummary& summary) {
  const auto access_shape = regalloc_access_shape_name(summary);
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

RegallocObjectAccessSummary summarize_object_accesses(const bir::Function& function,
                                                      std::string_view source_name) {
  RegallocObjectAccessSummary summary;
  std::size_t instruction_index = 0;

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
        if (load->slot_name == source_name) {
          ++summary.direct_read_count;
          record_access(summary, instruction_index, RegallocAccessKind::DirectRead);
        }
        if (memory_address_targets_object(load->address, source_name)) {
          ++summary.addressed_access_count;
          record_access(summary, instruction_index, RegallocAccessKind::AddressedAccess);
        }
        ++instruction_index;
        continue;
      }

      if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst); store != nullptr) {
        if (store->slot_name == source_name) {
          ++summary.direct_write_count;
          record_access(summary, instruction_index, RegallocAccessKind::DirectWrite);
        }
        if (memory_address_targets_object(store->address, source_name)) {
          ++summary.addressed_access_count;
          record_access(summary, instruction_index, RegallocAccessKind::AddressedAccess);
        }
        ++instruction_index;
        continue;
      }

      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr) {
        ++instruction_index;
        continue;
      }
      summary.call_instruction_indices.push_back(instruction_index);
      for (const auto& arg : call->args) {
        if (named_value_targets_object(arg, source_name)) {
          ++summary.call_arg_exposure_count;
          record_access(summary, instruction_index, RegallocAccessKind::CallArgumentExposure);
        }
      }
      ++instruction_index;
    }
  }

  return summary;
}

bool crosses_call_boundary(const RegallocObjectAccessSummary& summary) {
  if (!summary.has_access_window ||
      summary.first_access_instruction_index == summary.last_access_instruction_index) {
    return false;
  }
  return std::any_of(summary.call_instruction_indices.begin(),
                     summary.call_instruction_indices.end(),
                     [&](std::size_t instruction_index) {
                       return instruction_index > summary.first_access_instruction_index &&
                              instruction_index < summary.last_access_instruction_index;
                     });
}

std::string_view regalloc_priority_bucket(std::string_view contract_kind,
                                          const RegallocObjectAccessSummary& summary) {
  if (contract_kind != "value_storage") {
    return "non_value_storage";
  }
  if (!summary.has_access_window ||
      summary.first_access_instruction_index == summary.last_access_instruction_index) {
    return "single_point_value";
  }
  if (crosses_call_boundary(summary)) {
    return "call_spanning_value";
  }
  return "multi_point_value";
}

std::string regalloc_assignment_readiness(std::string_view allocation_kind,
                                          std::string_view priority_bucket,
                                          const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    return "fixed_stack_only";
  }

  const std::string_view bucket_prefix = priority_bucket == "single_point_value"
                                             ? "single_point"
                                         : priority_bucket == "call_spanning_value"
                                             ? "call_spanning"
                                             : priority_bucket == "multi_point_value"
                                             ? "multi_point"
                                             : "register_candidate";
  return std::string(bucket_prefix) + "_" +
         std::string(regalloc_access_shape_suffix(summary)) + "_candidate";
}

std::string_view regalloc_preferred_register_pool(std::string_view allocation_kind,
                                                  std::string_view priority_bucket) {
  if (allocation_kind != "register_candidate") {
    return "fixed_stack_only";
  }
  if (priority_bucket == "call_spanning_value") {
    return "callee_saved_preferred";
  }
  return "caller_saved_preferred";
}

std::string_view regalloc_spill_pressure_hint(std::string_view allocation_kind,
                                              std::string_view priority_bucket,
                                              const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    return "fixed_stack_only";
  }
  if (summary.direct_read_count == 0) {
    return "write_only_spill_friendly";
  }
  if (priority_bucket == "call_spanning_value") {
    return "call_surviving_spill_costly";
  }
  if (priority_bucket == "multi_point_value") {
    return "repeat_use_spill_costly";
  }
  return "single_use_spill_friendly";
}

std::string_view regalloc_reload_cost_hint(std::string_view allocation_kind,
                                           std::string_view priority_bucket,
                                           const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "stack_address_exposed";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "stack_call_exposed";
    }
    return "fixed_stack_only";
  }
  if (summary.direct_read_count == 0) {
    return "reload_not_needed";
  }
  if (priority_bucket == "single_point_value") {
    return "single_use_reload_light";
  }
  if (priority_bucket == "call_spanning_value") {
    return "call_spanning_reload_heavy";
  }
  if (summary.direct_read_count > 1 || summary.direct_write_count > 1) {
    return "reuse_window_reload_amortized";
  }
  return "multi_point_reload_moderate";
}

std::string_view regalloc_materialization_timing_hint(std::string_view allocation_kind,
                                                      const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "materialize_via_address_exposure";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "materialize_for_call_exposure";
    }
    return "fixed_stack_only";
  }

  switch (summary.first_access_kind) {
    case RegallocAccessKind::DirectWrite:
      if (summary.last_access_kind == RegallocAccessKind::DirectRead) {
        return "materialize_after_write_before_read";
      }
      return "materialize_on_write_path";
    case RegallocAccessKind::DirectRead:
      if (summary.last_access_kind == RegallocAccessKind::DirectWrite) {
        return "materialize_at_first_read_then_update";
      }
      return "materialize_at_first_read";
    case RegallocAccessKind::AddressedAccess:
      return "materialize_via_address_exposure";
    case RegallocAccessKind::CallArgumentExposure:
      return "materialize_for_call_exposure";
    case RegallocAccessKind::None:
      break;
  }

  return "materialize_at_first_access";
}

std::string_view regalloc_spill_restore_locality_hint(
    std::string_view allocation_kind,
    const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "fixed_stack_address_anchor";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "fixed_stack_call_boundary_anchor";
    }
    return "fixed_stack_only";
  }

  if (!summary.has_access_window) {
    return "unobserved_access_window";
  }
  if (crosses_call_boundary(summary)) {
    return "call_split_reuse_window";
  }

  const auto access_window_width =
      summary.last_access_instruction_index - summary.first_access_instruction_index;
  if (access_window_width == 0) {
    return "single_instruction_reuse_window";
  }
  if (access_window_width == 1) {
    return "adjacent_instruction_reuse_window";
  }
  return "wide_reuse_window";
}

std::string_view regalloc_register_eligibility_hint(
    std::string_view allocation_kind,
    const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "register_ineligible_address_exposed";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "register_ineligible_call_exposed";
    }
    return "register_ineligible_fixed_stack";
  }

  if (!summary.has_access_window) {
    return "register_eligibility_unobserved";
  }
  if (crosses_call_boundary(summary)) {
    if (summary.direct_write_count != 0 && summary.direct_read_count == 0) {
      return "register_eligible_call_writeback_window";
    }
    return "register_eligible_call_preserved_window";
  }

  const auto access_shape = regalloc_access_shape_name(summary);
  const auto access_window_width =
      summary.last_access_instruction_index - summary.first_access_instruction_index;
  if (access_shape == "direct_write_only") {
    if (access_window_width == 0) {
      return "register_eligible_single_write_buffer";
    }
    return "register_eligible_local_write_buffer";
  }
  if (access_shape == "direct_read_only") {
    if (access_window_width == 0) {
      return "register_eligible_single_read_cache";
    }
    return "register_eligible_local_read_cache";
  }
  if (access_shape == "direct_read_write") {
    if (access_window_width <= 1) {
      return "register_eligible_tight_reuse_window";
    }
    return "register_eligible_wide_reuse_window";
  }
  return "register_eligibility_requires_future_analysis";
}

std::string_view regalloc_spill_sync_hint(std::string_view allocation_kind,
                                          const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "fixed_stack_memory_authoritative";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "fixed_stack_call_boundary_authoritative";
    }
    return "fixed_stack_only";
  }

  if (!summary.has_access_window) {
    return "spill_sync_unobserved";
  }

  if (summary.direct_write_count == 0) {
    if (crosses_call_boundary(summary)) {
      return "restore_only_call_window";
    }
    if (summary.direct_read_count <= 1) {
      return "restore_only_single_use";
    }
    return "restore_only_reuse_window";
  }

  if (summary.direct_read_count == 0) {
    if (crosses_call_boundary(summary)) {
      return "writeback_only_call_window";
    }
    if (summary.direct_write_count <= 1) {
      return "writeback_only_single_definition";
    }
    return "writeback_only_redefinition_window";
  }

  if (crosses_call_boundary(summary)) {
    return "bidirectional_sync_call_window";
  }
  return "bidirectional_sync_local_window";
}

std::string_view regalloc_home_slot_stability_hint(
    std::string_view allocation_kind,
    const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "memory_anchor_home_slot";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "call_boundary_anchor_home_slot";
    }
    return "fixed_stack_only";
  }

  if (!summary.has_access_window) {
    return "home_slot_unobserved";
  }

  if (crosses_call_boundary(summary)) {
    if (summary.direct_write_count == 0) {
      return "call_preserved_read_home_slot";
    }
    if (summary.direct_read_count == 0) {
      return "call_preserved_write_home_slot";
    }
    return "call_preserved_read_write_home_slot";
  }

  const auto access_window_width =
      summary.last_access_instruction_index - summary.first_access_instruction_index;
  if (summary.direct_write_count == 0) {
    if (access_window_width == 0) {
      return "single_use_read_home_slot";
    }
    if (access_window_width == 1) {
      return "adjacent_read_home_slot";
    }
    return "wide_read_home_slot";
  }

  if (summary.direct_read_count == 0) {
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

std::string_view regalloc_eviction_friction_hint(
    std::string_view allocation_kind,
    const RegallocObjectAccessSummary& summary) {
  if (allocation_kind != "register_candidate") {
    if (summary.addressed_access_count != 0) {
      return "eviction_fixed_stack_memory_anchor";
    }
    if (summary.call_arg_exposure_count != 0) {
      return "eviction_fixed_stack_call_anchor";
    }
    return "eviction_fixed_stack_only";
  }

  if (!summary.has_access_window) {
    return "eviction_friction_unobserved";
  }

  if (crosses_call_boundary(summary)) {
    if (summary.direct_write_count == 0) {
      return "eviction_friction_call_reload_guarded";
    }
    if (summary.direct_read_count == 0) {
      return "eviction_friction_call_writeback_guarded";
    }
    return "eviction_friction_call_sync_heavy";
  }

  const auto access_window_width =
      summary.last_access_instruction_index - summary.first_access_instruction_index;
  if (summary.direct_write_count == 0) {
    if (access_window_width == 0) {
      return "eviction_friction_single_read_light";
    }
    return "eviction_friction_local_read_buffered";
  }
  if (summary.direct_read_count == 0) {
    if (access_window_width == 0) {
      return "eviction_friction_single_write_light";
    }
    return "eviction_friction_local_write_buffered";
  }
  if (access_window_width <= 1) {
    return "eviction_friction_tight_reuse_balanced";
  }
  return "eviction_friction_wide_reuse_guarded";
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

int regalloc_assignment_readiness_rank(std::string_view assignment_readiness) {
  if (assignment_readiness == "call_spanning_read_write_candidate") {
    return 0;
  }
  if (assignment_readiness == "call_spanning_read_candidate") {
    return 1;
  }
  if (assignment_readiness == "call_spanning_write_candidate") {
    return 2;
  }
  if (assignment_readiness == "multi_point_read_write_candidate") {
    return 3;
  }
  if (assignment_readiness == "multi_point_read_candidate") {
    return 4;
  }
  if (assignment_readiness == "multi_point_write_candidate") {
    return 5;
  }
  if (assignment_readiness == "single_point_read_candidate") {
    return 6;
  }
  if (assignment_readiness == "single_point_write_candidate") {
    return 7;
  }
  return 8;
}

int regalloc_eviction_friction_rank(std::string_view eviction_friction_hint) {
  if (eviction_friction_hint == "eviction_friction_call_sync_heavy") {
    return 0;
  }
  if (eviction_friction_hint == "eviction_friction_call_reload_guarded") {
    return 1;
  }
  if (eviction_friction_hint == "eviction_friction_call_writeback_guarded") {
    return 2;
  }
  if (eviction_friction_hint == "eviction_friction_tight_reuse_balanced") {
    return 3;
  }
  if (eviction_friction_hint == "eviction_friction_local_read_buffered") {
    return 4;
  }
  if (eviction_friction_hint == "eviction_friction_local_write_buffered") {
    return 5;
  }
  if (eviction_friction_hint == "eviction_friction_wide_reuse_guarded") {
    return 6;
  }
  if (eviction_friction_hint == "eviction_friction_single_read_light") {
    return 7;
  }
  if (eviction_friction_hint == "eviction_friction_single_write_light") {
    return 8;
  }
  return 9;
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

}  // namespace

void run_regalloc(PreparedLirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("regalloc");
  module.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message = "regalloc skeleton is wired; no physical register assignment is performed yet",
  });
}

void run_regalloc(PreparedBirModule& module, const PrepareOptions& options) {
  (void)options;
  module.completed_phases.push_back("regalloc");
  module.regalloc.functions.clear();
  module.regalloc.functions.reserve(module.module.functions.size());

  for (const auto& function : module.module.functions) {
    PreparedRegallocFunction prepared_function{
        .function_name = function.name,
    };
    for (const auto& object : module.liveness.objects) {
      if (object.function_name != function.name) {
        continue;
      }
      const auto summary = summarize_object_accesses(function, object.source_name);
      const std::string allocation_kind(regalloc_allocation_kind(object.contract_kind));
      const std::string priority_bucket(regalloc_priority_bucket(object.contract_kind, summary));
      const std::string preferred_register_pool(
          regalloc_preferred_register_pool(allocation_kind, priority_bucket));
      const std::string spill_pressure_hint(
          regalloc_spill_pressure_hint(allocation_kind, priority_bucket, summary));
      const std::string reload_cost_hint(
          regalloc_reload_cost_hint(allocation_kind, priority_bucket, summary));
      const std::string materialization_timing_hint(
          regalloc_materialization_timing_hint(allocation_kind, summary));
      const std::string spill_restore_locality_hint(
          regalloc_spill_restore_locality_hint(allocation_kind, summary));
      const std::string register_eligibility_hint(
          regalloc_register_eligibility_hint(allocation_kind, summary));
      const std::string spill_sync_hint(regalloc_spill_sync_hint(allocation_kind, summary));
      const std::string home_slot_stability_hint(
          regalloc_home_slot_stability_hint(allocation_kind, summary));
      const std::string eviction_friction_hint(
          regalloc_eviction_friction_hint(allocation_kind, summary));
      const std::string assignment_readiness(
          regalloc_assignment_readiness(allocation_kind, priority_bucket, summary));
      prepared_function.objects.push_back(PreparedRegallocObject{
          .function_name = object.function_name,
          .source_name = object.source_name,
          .source_kind = object.source_kind,
          .contract_kind = object.contract_kind,
          .allocation_kind = allocation_kind,
          .priority_bucket = priority_bucket,
          .preferred_register_pool = preferred_register_pool,
          .spill_pressure_hint = spill_pressure_hint,
          .reload_cost_hint = reload_cost_hint,
          .materialization_timing_hint = materialization_timing_hint,
          .spill_restore_locality_hint = spill_restore_locality_hint,
          .register_eligibility_hint = register_eligibility_hint,
          .spill_sync_hint = spill_sync_hint,
          .home_slot_stability_hint = home_slot_stability_hint,
          .eviction_friction_hint = eviction_friction_hint,
          .assignment_readiness = assignment_readiness,
          .access_shape = std::string(regalloc_access_shape_name(summary)),
          .first_access_kind = std::string(regalloc_access_kind_name(summary.first_access_kind)),
          .last_access_kind = std::string(regalloc_access_kind_name(summary.last_access_kind)),
          .direct_read_count = summary.direct_read_count,
          .direct_write_count = summary.direct_write_count,
          .addressed_access_count = summary.addressed_access_count,
          .call_arg_exposure_count = summary.call_arg_exposure_count,
          .has_access_window = summary.has_access_window,
          .first_access_instruction_index = summary.first_access_instruction_index,
          .last_access_instruction_index = summary.last_access_instruction_index,
          .crosses_call_boundary = crosses_call_boundary(summary),
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
                       const int lhs_readiness_rank =
                           regalloc_assignment_readiness_rank(lhs->assignment_readiness);
                       const int rhs_readiness_rank =
                           regalloc_assignment_readiness_rank(rhs->assignment_readiness);
                       if (lhs_readiness_rank != rhs_readiness_rank) {
                         return lhs_readiness_rank < rhs_readiness_rank;
                       }
                       const int lhs_friction_rank =
                           regalloc_eviction_friction_rank(lhs->eviction_friction_hint);
                       const int rhs_friction_rank =
                           regalloc_eviction_friction_rank(rhs->eviction_friction_hint);
                       if (lhs_friction_rank != rhs_friction_rank) {
                         return lhs_friction_rank < rhs_friction_rank;
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
    if (!prepared_function.objects.empty()) {
      module.regalloc.functions.push_back(std::move(prepared_function));
    }
  }

  module.notes.push_back(PrepareNote{
      .phase = "regalloc",
      .message =
          "regalloc now groups prepared liveness objects per function and classifies them as "
          "register_candidate or fixed_stack_storage contracts, plus target-neutral priority "
          "buckets for single-point, multi-point, and call-spanning value-storage objects, "
          "preferred register pools that map call-spanning value storage toward callee-saved "
          "pressure and other register candidates toward caller-saved pressure, "
          "spill-pressure hints that distinguish single-use, repeat-use, call-surviving, "
          "write-only, and fixed-stack cases from current prepared facts, "
          "reload-cost hints that distinguish single-use, reuse-window, call-spanning, "
          "reload-free, and fixed-stack exposure cases from those same prepared facts, "
          "materialization-timing hints keyed by first/last access order plus fixed-stack "
          "exposure kind, spill/restore-locality hints keyed by access-window width plus "
          "fixed-stack exposure kind, register-eligibility hints keyed by contract kind, "
          "access shape, and call-crossing or local-window reuse facts, spill-sync hints "
          "keyed by observed read/write direction plus fixed-stack exposure kind, home-slot "
          "stability hints keyed by access-window width and read/write direction plus "
          "fixed-stack exposure kind, eviction-friction hints keyed by the same prepared "
          "access-window and call-crossing facts without naming target registers, "
          "allocation-sequence decisions that consume the current prepared contract into "
          "stabilize_across_calls, stabilize_local_reuse, and opportunistic_single_point "
          "staging for register candidates, plus first-pass reservation decisions that "
          "turn those stages into target-neutral call-preserved, local-reuse, or "
          "single-point register attempts with scope, home-slot, and sync policies, "
          "assignment-readiness cues built from those buckets plus compact access-shape "
          "summaries, first and last access-kind cues, "
          "direct read/write, addressed-access, and call-argument exposure counts, and "
          "instruction-order access windows and call-crossing cues for downstream prepared-BIR "
          "consumers; physical register assignment remains future work",
  });
}

}  // namespace c4c::backend::prepare
