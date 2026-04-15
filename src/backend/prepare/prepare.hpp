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
};

struct PreparedLiveness {
  std::vector<PreparedLivenessObject> objects;
};

struct PreparedRegallocObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  std::string contract_kind;
  std::string allocation_kind;
  std::string priority_bucket;
  std::string preferred_register_pool;
  std::string spill_pressure_hint;
  std::string reload_cost_hint;
  std::string materialization_timing_hint;
  std::string spill_restore_locality_hint;
  std::string register_eligibility_hint;
  std::string spill_sync_hint;
  std::string home_slot_stability_hint;
  std::string eviction_friction_hint;
  std::string assignment_readiness;
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

struct PreparedRegallocFunction {
  std::string function_name;
  std::vector<PreparedRegallocObject> objects;
  std::vector<PreparedRegallocAllocationDecision> allocation_sequence;
  std::size_t register_candidate_count = 0;
  std::size_t fixed_stack_storage_count = 0;
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
