#pragma once

#include "analysis.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::stack_layout {

struct EntryAllocaSlotPlan {
  std::string alloca_name;
  bool needs_stack_slot = true;
  bool remove_following_entry_store = false;
  std::optional<std::size_t> coalesced_block;
  std::optional<std::size_t> assigned_slot;
};

struct ParamAllocaSlotPlan {
  std::string alloca_name;
  std::string param_name;
  bool needs_stack_slot = true;
};

struct StackLayoutPlanBundle {
  StackLayoutAnalysis analysis;
  std::vector<EntryAllocaSlotPlan> entry_alloca_plans;
  std::vector<ParamAllocaSlotPlan> param_alloca_plans;
};

struct EntryAllocaRewritePatch {
  std::vector<c4c::codegen::lir::LirInst> alloca_insts;
  std::vector<std::pair<std::string, std::string>> canonical_allocas;
};

enum class EntryAllocaRewriteLivenessSource {
  RawLirBackendCfg,
  PerFunctionBir,
};

enum class EntryAllocaRewriteStackLayoutSource {
  RawLirFunction,
  EntryAllocasAndBackendCfg,
};

struct PreparedEntryAllocaStackLayoutMetadata {
  std::vector<StackLayoutSignatureParam> signature_params;
  std::optional<std::string> return_type;
  bool is_variadic = false;
  std::vector<StackLayoutCallResultInput> call_results;
};

struct PreparedEntryAllocaStackLayoutClassificationInput {
  std::optional<std::vector<std::string>> escaped_entry_allocas;
  std::optional<std::vector<EntryAllocaUseBlocks>> entry_alloca_use_blocks;
  std::optional<std::vector<EntryAllocaFirstAccess>> entry_alloca_first_accesses;
};

struct PreparedEntryAllocaRewriteMetadata {
  std::vector<EntryAllocaInput> entry_allocas;
};

struct EntryAllocaPairedStorePlanInfo {
  bool has_store = false;
  bool is_zero_initializer = false;
  std::optional<std::string> param_name;
};

struct EntryAllocaPlanInput {
  std::string alloca_name;
  std::string type_str;
  int align = 0;
  EntryAllocaPairedStorePlanInfo paired_store;
};

struct EntryAllocaPlanningInput {
  std::vector<EntryAllocaPlanInput> entry_allocas;
  std::optional<std::vector<std::string>> escaped_entry_allocas;
  std::optional<std::vector<EntryAllocaUseBlocks>> entry_alloca_use_blocks;
  std::optional<std::vector<EntryAllocaFirstAccess>> entry_alloca_first_accesses;
};

struct EntryAllocaRewriteInput {
  std::vector<EntryAllocaInput> entry_allocas;
  std::optional<std::vector<std::string>> escaped_entry_allocas;
  std::optional<std::vector<EntryAllocaUseBlocks>> entry_alloca_use_blocks;
  std::optional<std::vector<EntryAllocaFirstAccess>> entry_alloca_first_accesses;
};

struct PreparedEntryAllocaFunctionInputs {
  PreparedEntryAllocaStackLayoutClassificationInput stack_layout_classification;
  PreparedEntryAllocaStackLayoutMetadata stack_layout_metadata;
  PreparedEntryAllocaRewriteMetadata rewrite_metadata;
  EntryAllocaPlanningInput planning_input;
  std::optional<BackendCfgLivenessFunction> backend_cfg_liveness;
  std::optional<LivenessInput> liveness_input;
  EntryAllocaRewriteLivenessSource liveness_source =
      EntryAllocaRewriteLivenessSource::RawLirBackendCfg;
  EntryAllocaRewriteStackLayoutSource stack_layout_source =
      EntryAllocaRewriteStackLayoutSource::RawLirFunction;
};

struct EntryAllocaRewriteInputs {
  LivenessInput liveness_input;
  StackLayoutInput stack_layout_input;
  EntryAllocaRewriteInput rewrite_input;
  EntryAllocaPlanningInput planning_input;
  EntryAllocaRewriteLivenessSource liveness_source =
      EntryAllocaRewriteLivenessSource::RawLirBackendCfg;
  EntryAllocaRewriteStackLayoutSource stack_layout_source =
      EntryAllocaRewriteStackLayoutSource::RawLirFunction;
};

struct PreparedEntryAllocaRewriteOnlyInputs {
  LivenessInput liveness_input;
  EntryAllocaRewriteInput rewrite_input;
  EntryAllocaPlanningInput planning_input;
  EntryAllocaRewriteLivenessSource liveness_source =
      EntryAllocaRewriteLivenessSource::RawLirBackendCfg;
  EntryAllocaRewriteStackLayoutSource stack_layout_source =
      EntryAllocaRewriteStackLayoutSource::RawLirFunction;
};

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis);

// Compatibility wrapper around the narrower planning seam.
StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const StackLayoutInput& input,
    const RegAllocIntegrationResult& regalloc,
    const std::vector<PhysReg>& callee_saved_regs);

StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const LivenessInput& liveness_input,
    const EntryAllocaPlanningInput& planning_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

// Compatibility wrapper around the narrower planning seam.
StackLayoutPlanBundle build_stack_layout_plan_bundle(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

// Compatibility wrapper around the narrower rewrite seam.
EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const StackLayoutInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans);

EntryAllocaRewritePatch build_entry_alloca_rewrite_patch(
    const EntryAllocaRewriteInput& input,
    const std::vector<EntryAllocaSlotPlan>& plans);

// Compatibility wrapper around the narrower rewrite seam.
EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const StackLayoutInput& stack_layout_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

EntryAllocaRewritePatch prepare_entry_alloca_rewrite_patch(
    const LivenessInput& liveness_input,
    const EntryAllocaRewriteInput& rewrite_input,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

[[nodiscard]] std::optional<LivenessInput> try_lower_module_function_to_bir_liveness_input(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index);

[[nodiscard]] PreparedEntryAllocaFunctionInputs prepare_module_function_entry_alloca_preparation(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index);

[[nodiscard]] EntryAllocaRewriteInputs lower_prepared_entry_alloca_function_inputs(
    const PreparedEntryAllocaFunctionInputs& prepared_inputs);

[[nodiscard]] PreparedEntryAllocaRewriteOnlyInputs
lower_prepared_entry_alloca_rewrite_only_inputs(
    const PreparedEntryAllocaFunctionInputs& prepared_inputs);

// Compatibility-only wrapper that rehydrates the broader stack-layout view for
// direct-LIR emitters and tests. Production rewrite flows should prefer
// `prepare_module_function_entry_alloca_rewrite_only_inputs(...)`.
[[nodiscard]] EntryAllocaRewriteInputs prepare_module_function_entry_alloca_compat_inputs(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index);

[[nodiscard]] PreparedEntryAllocaRewriteOnlyInputs
prepare_module_function_entry_alloca_rewrite_only_inputs(
    const c4c::codegen::lir::LirModule& module,
    std::size_t function_index);

[[nodiscard]] c4c::codegen::lir::LirModule rewrite_module_entry_allocas(
    const c4c::codegen::lir::LirModule& module,
    const RegAllocConfig& regalloc_config,
    const std::vector<PhysReg>& asm_clobbered,
    const std::vector<PhysReg>& callee_saved_regs);

// Final raw-LIR mutation stays isolated here after planning/rewrite prep.
void apply_entry_alloca_rewrite_patch(
    c4c::codegen::lir::LirFunction& function,
    const EntryAllocaRewritePatch& patch);

// Compatibility wrapper around the narrower planning seam.
std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const EntryAllocaRewriteInput& input,
    const StackLayoutAnalysis& analysis);

std::vector<EntryAllocaSlotPlan> plan_entry_alloca_slots(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis);

// Compatibility wrapper around the narrower planning seam.
std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const StackLayoutInput& input,
    const StackLayoutAnalysis& analysis);

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const EntryAllocaRewriteInput& input,
    const StackLayoutAnalysis& analysis);

std::vector<ParamAllocaSlotPlan> plan_param_alloca_slots(
    const EntryAllocaPlanningInput& input,
    const StackLayoutAnalysis& analysis);

}  // namespace c4c::backend::stack_layout
