#include "prealloc.hpp"
#include "atomics.hpp"
#include "call_plans.hpp"
#include "dynamic_stack.hpp"
#include "f128_runtime_helpers.hpp"
#include "frame_plan.hpp"
#include "i128_runtime_helpers.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "label_identity.hpp"
#include "object_data.hpp"
#include "publication_plans.hpp"
#include "regalloc_placement_identity.hpp"
#include "special_carriers.hpp"
#include "storage_plans.hpp"
#include "variadic_entry_plans.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace c4c::backend::prepare {

void BirPreAlloc::note(std::string_view message) {
  prepared_.notes.push_back(PrepareNote{
      .phase = "prealloc",
      .message = std::string(message),
  });
}

PreparedBirModule BirPreAlloc::run() {
  note("prealloc owns the shared semantic-BIR to prealloc-BIR route before MIR lowering");
  run_legalize();
  run_stack_layout();
  run_liveness();
  run_out_of_ssa();
  populate_inline_asm_register_group_overrides(prepared_);
  run_regalloc();
  publish_contract_plans();
  return std::move(prepared_);
}

void BirPreAlloc::publish_contract_plans() {
  publish_prepared_bir_label_identity(prepared_);
  populate_regalloc_placement_identity(prepared_);
  populate_frame_plan(prepared_);
  populate_dynamic_stack_plan(prepared_);
  populate_call_plans(prepared_);
  populate_store_source_publication_plans(prepared_);
  populate_select_carrier_alias_identity(prepared_);
  populate_local_array_selected_proof_edge_paths(prepared_);
  populate_local_array_endpoint_bridges(prepared_);
  populate_local_array_ordered_effect_source_streams(prepared_);
  populate_variadic_entry_plans(prepared_);
  populate_frame_plan(prepared_);
  populate_storage_plans(prepared_);
  populate_prepared_object_data_plans(prepared_);
  populate_i128_carriers(prepared_);
  populate_f128_carriers(prepared_);
  populate_atomic_operations(prepared_);
  populate_intrinsic_carriers(prepared_);
  populate_inline_asm_carriers(prepared_);
  populate_f128_runtime_helper_facts(prepared_);
  populate_i128_runtime_helper_lanes(prepared_);
}

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options) {
  return BirPreAlloc(module, target_profile, options).run();
}

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options) {
  return prepare_semantic_bir_module_with_options(module, target_profile, options);
}

}  // namespace c4c::backend::prepare
