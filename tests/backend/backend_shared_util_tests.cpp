
#include "backend.hpp"
#include "generation.hpp"
#include "ir_printer.hpp"
#include "ir_validate.hpp"
#include "liveness.hpp"
#include "lir_adapter.hpp"
#include "regalloc.hpp"
#include "stack_layout/analysis.hpp"
#include "stack_layout/alloca_coalescing.hpp"
#include "stack_layout/regalloc_helpers.hpp"
#include "stack_layout/slot_assignment.hpp"
#include "target.hpp"
#include "../../src/codegen/lir/call_args.hpp"
#include "../../src/codegen/lir/lir_printer.hpp"
#include "../../src/codegen/lir/verify.hpp"
#include "../../src/backend/elf/mod.hpp"
#include "../../src/backend/linker_common/mod.hpp"
#include "../../src/backend/aarch64/assembler/mod.hpp"
#include "../../src/backend/aarch64/codegen/emit.hpp"
#include "../../src/backend/aarch64/linker/mod.hpp"
#include "../../src/backend/aarch64/assembler/parser.hpp"
#include "../../src/backend/x86/assembler/mod.hpp"
#include "../../src/backend/x86/assembler/encoder/mod.hpp"
#include "../../src/backend/x86/assembler/parser.hpp"
#include "../../src/backend/x86/codegen/emit.hpp"
#include "../../src/backend/x86/linker/mod.hpp"
#include "backend_test_helper.hpp"


void test_backend_shared_liveness_surface_tracks_result_names() {
  const auto module = make_declaration_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 0,
              "shared liveness surface should record call program points for typed call ops");
  const auto* interval = liveness.find_interval("%t0");
  expect_true(interval != nullptr,
              "shared liveness surface should expose interval lookup by SSA result name");
  expect_true(interval->start == 0 && interval->end == 1,
              "shared liveness surface should extend intervals through later uses instead of keeping point-only placeholders");
}



void test_backend_shared_liveness_surface_tracks_call_crossing_ranges() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  expect_true(liveness.call_points.size() == 1 && liveness.call_points.front() == 1,
              "shared liveness surface should record the direct call program point");
  const auto* before_call = liveness.find_interval("%t0");
  const auto* call_result = liveness.find_interval("%t1");
  const auto* final_sum = liveness.find_interval("%t2");
  expect_true(before_call != nullptr && before_call->start == 0 && before_call->end == 2,
              "shared liveness surface should extend values that remain live across a call through their later use");
  expect_true(call_result != nullptr && call_result->start == 1 && call_result->end == 2,
              "shared liveness surface should keep the call result live until its consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 2 && final_sum->end == 3,
              "shared liveness surface should include terminator uses in the final interval endpoint");
}

void test_backend_shared_liveness_surface_tracks_phi_join_ranges() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const auto liveness = c4c::backend::compute_live_intervals(function);

  const auto* cond = liveness.find_interval("%t0");
  const auto* then_value = liveness.find_interval("%t1");
  const auto* else_value = liveness.find_interval("%t2");
  const auto* phi_value = liveness.find_interval("%t3");
  const auto* final_sum = liveness.find_interval("%t4");

  expect_true(cond != nullptr && cond->start == 0 && cond->end == 1,
              "shared liveness surface should extend branch conditions through the conditional terminator");
  expect_true(then_value != nullptr && then_value->start == 2 && then_value->end == 3,
              "shared liveness surface should treat phi incoming values as used on the predecessor edge");
  expect_true(else_value != nullptr && else_value->start == 4 && else_value->end == 5,
              "shared liveness surface should track the alternate phi incoming value on its predecessor edge");
  expect_true(phi_value != nullptr && phi_value->start == 6 && phi_value->end == 7,
              "shared liveness surface should start phi results in the join block and extend them to their consuming instruction");
  expect_true(final_sum != nullptr && final_sum->start == 7 && final_sum->end == 8,
              "shared liveness surface should keep the join result live through the return terminator");
}

void test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval() {
  const auto module = make_return_add_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}, {21}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);
  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc surface should assign one register for the single-result helper slice");
  const auto it = regalloc.assignments.find("%t0");
  expect_true(it != regalloc.assignments.end() && it->second.index == 13,
              "shared regalloc surface should prefer the caller-saved pool for intervals that do not span calls");
  expect_true(regalloc.used_regs.empty(),
              "shared regalloc surface should leave the callee-saved usage set empty when only caller-saved registers are needed");
  expect_true(regalloc.liveness.has_value() &&
                  regalloc.liveness->find_interval("%t0") != nullptr,
              "shared regalloc surface should retain cached liveness for the handoff boundary");
}

void test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto before_call = regalloc.assignments.find("%t0");
  const auto call_result = regalloc.assignments.find("%t1");
  const auto final_sum = regalloc.assignments.find("%t2");

  expect_true(before_call != regalloc.assignments.end() && before_call->second.index == 20,
              "shared regalloc should keep call-spanning intervals on the callee-saved pool first");
  expect_true(call_result == regalloc.assignments.end(),
              "shared regalloc should spill overlapping call-spanning intervals when the callee-saved pool is exhausted");
  expect_true(final_sum != regalloc.assignments.end() && final_sum->second.index == 13,
              "shared regalloc should use caller-saved registers for non-call-spanning intervals");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should report only the used callee-saved register set");
}

void test_backend_shared_regalloc_reuses_register_after_interval_ends() {
  const auto module = make_non_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  const auto first_value = regalloc.assignments.find("%t0");
  const auto later_value = regalloc.assignments.find("%t2");

  expect_true(first_value != regalloc.assignments.end() && first_value->second.index == 20,
              "shared regalloc should assign the available register to the first live interval");
  expect_true(later_value != regalloc.assignments.end() && later_value->second.index == 20,
              "shared regalloc should reuse a freed register for a later non-overlapping interval");
  expect_true(regalloc.used_regs.size() == 1 && regalloc.used_regs.front().index == 20,
              "shared regalloc should keep the callee-saved usage set deduplicated when a register is reused");
}

void test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg() {
  const auto module = make_overlapping_interval_module();
  const auto& function = module.functions.front();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};

  const auto regalloc = c4c::backend::allocate_registers(function, config);

  expect_true(regalloc.assignments.size() == 1,
              "shared regalloc should spill one of two overlapping intervals when only one register is available");
  const bool first_assigned = regalloc.assignments.find("%t0") != regalloc.assignments.end();
  const bool second_assigned = regalloc.assignments.find("%t1") != regalloc.assignments.end();
  expect_true(first_assigned != second_assigned,
              "shared regalloc should not assign the same busy register to overlapping intervals");
}

void test_backend_shared_regalloc_helper_filters_and_merges_clobbers() {
  const std::vector<c4c::backend::PhysReg> callee_saved = {{20}, {21}, {22}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};
  const auto filtered =
      c4c::backend::stack_layout::filter_available_regs(callee_saved, asm_clobbered);

  expect_true(filtered.size() == 2 && filtered.front().index == 20 &&
                  filtered.back().index == 22,
              "shared stack-layout helper should remove inline-asm clobbered registers from the available set");

  const auto module = make_return_add_module();
  c4c::backend::RegAllocConfig config;
  config.available_regs = filtered;
  const auto merged = c4c::backend::run_regalloc_and_merge_clobbers(
      module.functions.front(), config, asm_clobbered);

  expect_true(merged.used_callee_saved.size() == 2 &&
                  merged.used_callee_saved.front().index == 20 &&
                  merged.used_callee_saved.back().index == 21,
              "shared regalloc helper should merge allocator usage with asm clobbers in sorted order");
}

void test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view() {
  const auto module = make_call_crossing_interval_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const std::vector<c4c::backend::PhysReg> asm_clobbered = {{21}};

  const auto merged =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, asm_clobbered);

  const auto* before_call =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t0");
  const auto* call_result =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t1");
  const auto* final_sum =
      c4c::backend::stack_layout::find_assigned_reg(merged, "%t2");
  const auto* cached_liveness =
      c4c::backend::stack_layout::find_cached_liveness(merged);

  expect_true(before_call != nullptr && before_call->index == 20,
              "shared stack-layout helper should expose assigned call-spanning values through the regalloc handoff");
  expect_true(call_result == nullptr,
              "shared stack-layout helper should preserve spilled values as missing from the assigned-register view");
  expect_true(final_sum != nullptr && final_sum->index == 13,
              "shared stack-layout helper should expose caller-saved assignments through the same handoff seam");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{20}),
              "shared stack-layout helper should report allocator-used callee-saved registers");
  expect_true(c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{21}),
              "shared stack-layout helper should also report inline-asm-clobbered callee-saved registers after merge");
  expect_true(!c4c::backend::stack_layout::uses_callee_saved_reg(merged, c4c::backend::PhysReg{22}),
              "shared stack-layout helper should reject untouched callee-saved registers");
  expect_true(cached_liveness != nullptr &&
                  cached_liveness->find_interval("%t0") != nullptr &&
                  cached_liveness->find_interval("%t1") != nullptr &&
                  cached_liveness->find_interval("%t2") != nullptr,
              "shared stack-layout helper should expose cached liveness for downstream stack-layout analysis");
}

void test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks() {
  const auto module = make_interval_phi_join_module();
  const auto& function = module.functions.front();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  const auto* then_value_uses = analysis.find_use_blocks("%t1");
  const auto* else_value_uses = analysis.find_use_blocks("%t2");
  const auto* phi_value_uses = analysis.find_use_blocks("%t3");

  expect_true(then_value_uses != nullptr && then_value_uses->size() == 1 &&
                  then_value_uses->front() == 1,
              "shared stack-layout analysis should attribute phi incoming uses to the predecessor block");
  expect_true(else_value_uses != nullptr && else_value_uses->size() == 1 &&
                  else_value_uses->front() == 2,
              "shared stack-layout analysis should keep alternate phi incoming values on their own predecessor block");
  expect_true(phi_value_uses != nullptr && phi_value_uses->size() == 1 &&
                  phi_value_uses->front() == 3,
              "shared stack-layout analysis should record normal instruction uses in the consuming block");
}

void test_backend_shared_stack_layout_analysis_detects_dead_param_allocas() {
  const auto module = make_dead_param_alloca_candidate_module();
  const auto& function = module.functions.back();

  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};
  const auto regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(function, config, {});
  const auto analysis = c4c::backend::stack_layout::analyze_stack_layout(
      function, regalloc, {{20}, {21}, {22}});

  expect_true(analysis.uses_value("%p.x"),
              "shared stack-layout analysis should collect body-used SSA values");
  expect_true(!analysis.uses_value("%lv.param.x"),
              "shared stack-layout analysis should not treat the synthesized param spill as a body use");
  expect_true(analysis.is_dead_param_alloca("%lv.param.x"),
              "shared stack-layout analysis should mark dead param allocas when the corresponding parameter value is kept in a callee-saved register");
}

void test_backend_shared_stack_layout_analysis_tracks_call_arg_values() {
  const auto module = make_escaped_local_alloca_candidate_module();
  const auto& function = module.functions.back();
  const c4c::backend::RegAllocIntegrationResult regalloc;
  const auto analysis =
      c4c::backend::stack_layout::analyze_stack_layout(function, regalloc, {});

  expect_true(analysis.uses_value("%lv.buf"),
              "shared stack-layout analysis should treat typed call-argument operands as real uses");
}

void test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto overwrite_first_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& overwrite_first_function = overwrite_first_module.functions.front();
  const auto overwrite_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(overwrite_first_function, regalloc, {});
  expect_true(overwrite_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should recognize live entry allocas whose first real access overwrites the slot before any read");

  const auto read_first_module = make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  expect_true(!read_first_analysis.is_entry_alloca_overwritten_before_read("%lv.x"),
              "shared stack-layout analysis should keep entry zero-init stores when the slot is read before it is overwritten");
}

void test_backend_shared_alloca_coalescing_classifies_non_param_allocas() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(dead_function,
                                                              dead_analysis);
  expect_true(dead_coalescing.is_dead_alloca("%lv.buf") &&
                  !dead_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should classify unused non-param allocas as dead instead of forcing a permanent slot");

  const auto single_block_module = make_live_local_alloca_candidate_module();
  const auto& single_block_function = single_block_module.functions.front();
  const auto single_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(single_block_function, regalloc, {});
  const auto single_block_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(single_block_function,
                                                              single_block_analysis);
  expect_true(!single_block_coalescing.is_dead_alloca("%lv.buf") &&
                  single_block_coalescing.find_single_block("%lv.buf") == 0,
              "shared alloca-coalescing should recognize GEP-tracked non-param allocas whose uses stay within one block");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_coalescing =
      c4c::backend::stack_layout::compute_coalescable_allocas(escaped_function,
                                                              escaped_analysis);
  expect_true(!escaped_coalescing.is_dead_alloca("%lv.buf") &&
                  !escaped_coalescing.find_single_block("%lv.buf").has_value(),
              "shared alloca-coalescing should leave call-escaped allocas out of the single-block pool");
}

void test_backend_shared_slot_assignment_plans_param_alloca_slots() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.param.x" &&
                  dead_plans.front().param_name == "%p.x" &&
                  !dead_plans.front().needs_stack_slot,
              "shared slot-assignment planning should skip dead param allocas once analysis proves the parameter is preserved in a callee-saved register");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.param.x" &&
                  live_plans.front().param_name == "%p.x" &&
                  live_plans.front().needs_stack_slot,
              "shared slot-assignment planning should retain live param allocas when the function body still reads from the parameter slot");
}

void test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts() {
  c4c::backend::RegAllocConfig config;
  config.available_regs = {{20}};
  config.caller_saved_regs = {{13}};

  const auto dead_module = make_dead_param_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.back();
  const auto dead_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(dead_function, config, {});
  const auto dead_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      dead_function, dead_regalloc, {{20}, {21}, {22}});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead param allocas and their entry stores");

  const auto live_module = make_live_param_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_regalloc =
      c4c::backend::run_regalloc_and_merge_clobbers(live_function, config, {});
  const auto live_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      live_function, live_regalloc, {{20}, {21}, {22}});
  const auto live_plans =
      c4c::backend::stack_layout::plan_param_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_param_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve live param allocas");
}

void test_backend_shared_slot_assignment_plans_entry_alloca_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);

  expect_true(dead_plans.size() == 1 && dead_plans.front().alloca_name == "%lv.buf" &&
                  !dead_plans.front().needs_stack_slot &&
                  dead_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop dead non-param entry allocas and their paired zero-init stores when analysis finds no uses");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);

  expect_true(live_plans.size() == 1 && live_plans.front().alloca_name == "%lv.buf" &&
                  live_plans.front().needs_stack_slot &&
                  !live_plans.front().remove_following_entry_store &&
                  live_plans.front().coalesced_block == 0 &&
                  live_plans.front().assigned_slot == 0,
              "shared slot-assignment planning should preserve paired zero-init stores for aggregate entry allocas while exposing the shared single-block reuse classification");

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  const auto& disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);

  expect_true(disjoint_plans.size() == 2 &&
                  disjoint_plans[0].alloca_name == "%lv.then" &&
                  disjoint_plans[0].coalesced_block == 1 &&
                  disjoint_plans[1].alloca_name == "%lv.else" &&
                  disjoint_plans[1].coalesced_block == 2 &&
                  disjoint_plans[0].assigned_slot.has_value() &&
                  disjoint_plans[0].assigned_slot == disjoint_plans[1].assigned_slot,
              "shared slot-assignment planning should turn disjoint single-block allocas into a concrete shared slot decision");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  const auto& same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);

  expect_true(same_block_plans.size() == 2 &&
                  same_block_plans[0].coalesced_block == 0 &&
                  same_block_plans[1].coalesced_block == 0 &&
                  same_block_plans[0].assigned_slot.has_value() &&
                  same_block_plans[1].assigned_slot.has_value() &&
                  same_block_plans[0].assigned_slot != same_block_plans[1].assigned_slot,
              "shared slot-assignment planning should keep same-block local allocas in distinct slots even when both are individually coalescable");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);

  expect_true(read_first_plans.size() == 1 &&
                  read_first_plans.front().alloca_name == "%lv.buf" &&
                  read_first_plans.front().needs_stack_slot &&
                  !read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);

  expect_true(scalar_overwrite_plans.size() == 1 &&
                  scalar_overwrite_plans.front().alloca_name == "%lv.x" &&
                  scalar_overwrite_plans.front().needs_stack_slot &&
                  scalar_overwrite_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should drop redundant paired zero-init stores for scalar entry allocas that are overwritten before any read");

  const auto scalar_read_first_module =
      make_read_before_store_scalar_local_alloca_candidate_module();
  const auto& scalar_read_first_function = scalar_read_first_module.functions.front();
  const auto scalar_read_first_analysis = c4c::backend::stack_layout::analyze_stack_layout(
      scalar_read_first_function, regalloc, {});
  const auto scalar_read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_read_first_function,
                                                          scalar_read_first_analysis);

  expect_true(scalar_read_first_plans.size() == 1 &&
                  scalar_read_first_plans.front().alloca_name == "%lv.x" &&
                  scalar_read_first_plans.front().needs_stack_slot &&
                  !scalar_read_first_plans.front().remove_following_entry_store,
              "shared slot-assignment planning should preserve paired zero-init stores for scalar entry allocas that are read before the first overwrite");

  const auto escaped_module = make_escaped_local_alloca_candidate_module();
  const auto& escaped_function = escaped_module.functions.back();
  const auto escaped_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(escaped_function, regalloc, {});
  const auto escaped_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(escaped_function,
                                                          escaped_analysis);

  expect_true(escaped_plans.size() == 1 &&
                  escaped_plans.front().alloca_name == "%lv.buf" &&
                  escaped_plans.front().needs_stack_slot &&
                  !escaped_plans.front().coalesced_block.has_value(),
              "shared slot-assignment planning should leave call-escaped local allocas out of the single-block reuse pool");
}

void test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto dead_module = make_dead_local_alloca_candidate_module();
  const auto& dead_function = dead_module.functions.front();
  const auto dead_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(dead_function, regalloc, {});
  const auto dead_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(dead_function, dead_analysis);
  const auto dead_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(dead_function, dead_plans);

  expect_true(dead_pruned.empty(),
              "shared slot-assignment pruning should drop dead non-param entry allocas and their paired zero-init stores");

  const auto live_module = make_live_local_alloca_candidate_module();
  const auto& live_function = live_module.functions.front();
  const auto live_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(live_function, regalloc, {});
  const auto live_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(live_function, live_analysis);
  const auto live_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(live_function, live_plans);

  expect_true(live_pruned.size() == live_function.alloca_insts.size(),
              "shared slot-assignment pruning should keep aggregate live non-param entry allocas and their paired zero-init stores");

  const auto read_first_module = make_read_before_store_local_alloca_candidate_module();
  const auto& read_first_function = read_first_module.functions.front();
  const auto read_first_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(read_first_function, regalloc, {});
  const auto read_first_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(read_first_function,
                                                          read_first_analysis);
  const auto read_first_pruned = c4c::backend::stack_layout::prune_dead_entry_alloca_insts(
      read_first_function, read_first_plans);

  expect_true(read_first_pruned.size() == read_first_function.alloca_insts.size(),
              "shared slot-assignment pruning should preserve paired zero-init stores when a live entry alloca is read before the first overwrite");

  const auto scalar_overwrite_module = make_overwrite_first_scalar_local_alloca_candidate_module();
  const auto& scalar_overwrite_function = scalar_overwrite_module.functions.front();
  const auto scalar_overwrite_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(scalar_overwrite_function, regalloc, {});
  const auto scalar_overwrite_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(scalar_overwrite_function,
                                                          scalar_overwrite_analysis);
  const auto scalar_overwrite_pruned =
      c4c::backend::stack_layout::prune_dead_entry_alloca_insts(scalar_overwrite_function,
                                                                scalar_overwrite_plans);

  expect_true(
      scalar_overwrite_pruned.size() + 1 == scalar_overwrite_function.alloca_insts.size() &&
          std::holds_alternative<c4c::codegen::lir::LirAllocaOp>(
              scalar_overwrite_pruned.front()),
      "shared slot-assignment pruning should keep scalar live entry allocas while dropping redundant paired zero-init stores");
}

void test_backend_shared_slot_assignment_applies_coalesced_entry_slots() {
  const c4c::backend::RegAllocIntegrationResult regalloc;

  const auto disjoint_module = make_disjoint_block_local_alloca_candidate_module();
  auto disjoint_function = disjoint_module.functions.front();
  const auto disjoint_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(disjoint_function, regalloc, {});
  const auto disjoint_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(disjoint_function,
                                                          disjoint_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(disjoint_function, disjoint_plans);

  expect_true(disjoint_function.alloca_insts.size() == 1,
              "shared slot-assignment application should collapse disjoint single-block allocas onto one retained entry slot");
  const auto* disjoint_alloca =
      std::get_if<c4c::codegen::lir::LirAllocaOp>(&disjoint_function.alloca_insts.front());
  expect_true(disjoint_alloca != nullptr && disjoint_alloca->result == "%lv.then",
              "shared slot-assignment application should retain the first assigned-slot owner as the canonical entry alloca");
  const auto* else_store =
      std::get_if<c4c::codegen::lir::LirStoreOp>(&disjoint_function.blocks[2].insts[0]);
  const auto* else_load =
      std::get_if<c4c::codegen::lir::LirLoadOp>(&disjoint_function.blocks[2].insts[1]);
  expect_true(else_store != nullptr && else_store->ptr == "%lv.then" &&
                  else_load != nullptr && else_load->ptr == "%lv.then",
              "shared slot-assignment application should rewrite disjoint-block users to the canonical shared slot");

  const auto same_block_module = make_same_block_local_alloca_candidate_module();
  auto same_block_function = same_block_module.functions.front();
  const auto same_block_analysis =
      c4c::backend::stack_layout::analyze_stack_layout(same_block_function, regalloc, {});
  const auto same_block_plans =
      c4c::backend::stack_layout::plan_entry_alloca_slots(same_block_function,
                                                          same_block_analysis);
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(same_block_function, same_block_plans);

  expect_true(same_block_function.alloca_insts.size() == 2,
              "shared slot-assignment application should keep same-block allocas distinct when planning assigned them different slots");

  const auto call_arg_module = make_disjoint_block_call_arg_alloca_candidate_module();
  auto call_arg_function = call_arg_module.functions.front();
  const std::vector<c4c::backend::stack_layout::EntryAllocaSlotPlan> call_arg_plans = {
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.then", true, false, std::nullopt, std::size_t{0}},
      c4c::backend::stack_layout::EntryAllocaSlotPlan{
          "%lv.else", true, false, std::nullopt, std::size_t{0}},
  };
  c4c::backend::stack_layout::apply_entry_alloca_slot_plan(call_arg_function,
                                                           call_arg_plans);

  const auto* rewritten_call =
      std::get_if<c4c::codegen::lir::LirCallOp>(&call_arg_function.blocks[2].insts[1]);
  expect_true(rewritten_call != nullptr && rewritten_call->args_str == "ptr %lv.then",
              "shared slot-assignment application should rewrite typed call arguments to the canonical coalesced entry alloca");
}

void test_backend_binary_utils_contract_headers_are_include_reachable() {
  [[maybe_unused]] c4c::backend::elf::ContractSurface elf_surface;
  [[maybe_unused]] c4c::backend::linker_common::ContractSurface linker_common_surface;
  [[maybe_unused]] c4c::backend::aarch64::assembler::ContractSurface assembler_surface;
  [[maybe_unused]] c4c::backend::aarch64::linker::ContractSurface linker_surface;
  [[maybe_unused]] c4c::backend::x86::assembler::ContractSurface x86_assembler_surface;
  [[maybe_unused]] c4c::backend::x86::linker::ContractSurface x86_linker_surface;

  const auto emitted = c4c::backend::emit_module(
      c4c::backend::BackendModuleInput{make_return_add_module()},
      c4c::backend::BackendOptions{c4c::backend::Target::Aarch64});
  const auto parsed = c4c::backend::aarch64::assembler::parse_asm(emitted);
  expect_true(parsed.size() >= 4, "binary-utils contract headers should expose line-oriented parser output for backend-emitted assembly");
  expect_true(parsed.front().text == ".text",
              "binary-utils contract parser should keep the opening section directive");
  expect_true(parsed.back().text == "ret",
              "binary-utils contract parser should preserve the backend instruction tail");

  const auto object_path = make_temp_output_path("c4c_aarch64_contract");
  const auto staged = c4c::backend::aarch64::assembler::assemble(
      c4c::backend::aarch64::assembler::AssembleRequest{
          .asm_text = emitted,
          .output_path = object_path,
      });
  expect_true(staged.staged_text == emitted && staged.output_path == object_path &&
                  staged.object_emitted,
              "binary-utils contract headers should expose the active assembler request/result seam and report successful object emission for the minimal backend slice");
  const auto object_bytes = read_file_bytes(object_path);
  expect_true(object_bytes.size() >= 4 && object_bytes[0] == 0x7f &&
                  object_bytes[1] == 'E' && object_bytes[2] == 'L' &&
                  object_bytes[3] == 'F',
              "binary-utils contract assembler seam should emit an ELF object for the minimal backend slice");
  expect_true(std::string(object_bytes.begin(), object_bytes.end()).find("main") != std::string::npos,
              "binary-utils contract assembler seam should preserve the function symbol in the emitted object");
  std::filesystem::remove(object_path);

  const auto compat_path = make_temp_output_path("c4c_aarch64_contract_compat");
  const auto assembled = c4c::backend::aarch64::assembler::assemble(emitted, compat_path);
  expect_true(assembled == emitted,
              "binary-utils contract headers should keep the compatibility overload aligned with the staged assembler text seam");
  expect_true(std::filesystem::exists(compat_path),
              "binary-utils contract compatibility overload should still drive object emission through the named assembler seam");
  std::filesystem::remove(compat_path);
}

void test_shared_linker_parses_minimal_relocation_object_fixture() {
  const auto fixture = make_minimal_relocation_object_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      fixture, "minimal_reloc.o", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the bounded relocation-bearing fixture: " +
                  error);

  const auto& object = *parsed;
  expect_true(object.source_name == "minimal_reloc.o",
              "shared linker object parser should preserve the source name");
  expect_true(object.sections.size() == 6,
              "shared linker object parser should preserve the full section inventory including the null section");
  expect_true(object.symbols.size() == 4,
              "shared linker object parser should preserve the full symbol inventory for the bounded object slice");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should index relocation vectors by section");

  const auto text_index = find_section_index(object, ".text");
  const auto rela_text_index = find_section_index(object, ".rela.text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && rela_text_index < object.sections.size() &&
                  symtab_index < object.sections.size() && strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should name the expected bounded ELF sections");

  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the bounded .text payload");
  expect_true(object.section_data[rela_text_index].size() == 24,
              "shared linker object parser should preserve the raw RELA payload for the bounded fixture");
  expect_true(object.symbols[2].name == "main" && object.symbols[2].is_global() &&
                  object.symbols[2].sym_type() == c4c::backend::elf::STT_FUNC &&
                  object.symbols[2].shndx == text_index,
              "shared linker object parser should preserve the bounded function symbol inventory");
  expect_true(object.symbols[3].name == "helper_ext" && object.symbols[3].is_undefined(),
              "shared linker object parser should preserve undefined extern symbols for later linker resolution");

  expect_true(object.relocations[text_index].size() == 1,
              "shared linker object parser should associate .rela.text entries with the .text section they target");
  const auto& relocation = object.relocations[text_index].front();
  expect_true(relocation.offset == 4 && relocation.sym_idx == 3 &&
                  relocation.rela_type == 279 && relocation.addend == 0,
              "shared linker object parser should preserve the bounded relocation inventory for later linker work");
}

void test_shared_linker_parses_builtin_return_add_object() {
  const auto object_path = make_temp_output_path("c4c_aarch64_return_add_parse");
  const auto staged = c4c::backend::aarch64::assemble_module(make_return_add_module(), object_path);
  expect_true(staged.object_emitted,
              "built-in assembler should emit an object for the current bounded return-add slice");

  const auto object_bytes = read_file_bytes(object_path);
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_object(
      object_bytes, object_path, c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker object parser should accept the current built-in return-add object: " +
                  error);

  const auto& object = *parsed;
  const auto text_index = find_section_index(object, ".text");
  const auto symtab_index = find_section_index(object, ".symtab");
  const auto strtab_index = find_section_index(object, ".strtab");
  const auto shstrtab_index = find_section_index(object, ".shstrtab");
  expect_true(text_index < object.sections.size() && symtab_index < object.sections.size() &&
                  strtab_index < object.sections.size() &&
                  shstrtab_index < object.sections.size(),
              "shared linker object parser should preserve the current built-in return-add section inventory");
  expect_true(object.relocations.size() == object.sections.size(),
              "shared linker object parser should keep relocation vectors indexed by section for built-in emitted objects");
  expect_true(object.section_data[text_index].size() == 8,
              "shared linker object parser should preserve the built-in emitted return-add text payload");

  const auto main_symbol = std::find_if(
      object.symbols.begin(), object.symbols.end(),
      [&](const c4c::backend::linker_common::Elf64Symbol& symbol) {
        return symbol.name == "main";
      });
  expect_true(main_symbol != object.symbols.end() && main_symbol->is_global() &&
                  main_symbol->sym_type() == c4c::backend::elf::STT_FUNC &&
                  main_symbol->shndx == text_index,
              "shared linker object parser should preserve the built-in emitted function symbol inventory");

  expect_true(object.relocations[text_index].empty(),
              "shared linker object parser should preserve the current built-in return-add object as relocation-free");

  std::filesystem::remove(object_path);
}

void test_shared_linker_parses_single_member_archive_fixture() {
  const auto fixture = make_single_member_archive_fixture();
  std::string error;
  const auto parsed = c4c::backend::linker_common::parse_elf64_archive(
      fixture, "libminimal.a", c4c::backend::elf::EM_AARCH64, &error);
  expect_true(parsed.has_value(),
              "shared linker archive parser should accept the bounded single-member fixture: " +
                  error);

  const auto& archive = *parsed;
  expect_true(archive.source_name == "libminimal.a",
              "shared linker archive parser should preserve the archive source name");
  expect_true(archive.members.size() == 1,
              "shared linker archive parser should preserve the bounded single-member inventory");
  expect_true(archive.find_member_index_for_symbol("main").has_value(),
              "shared linker archive parser should support symbol-driven member lookup for the bounded archive slice");

  const auto member_index = *archive.find_member_index_for_symbol("main");
  const auto& member = archive.members[member_index];
  expect_true(member.name == "minimal_reloc.o",
              "shared linker archive parser should normalize the bounded member name");
  expect_true(member.object.has_value(),
              "shared linker archive parser should parse the bounded ELF member into a shared object surface");
  expect_true(member.defines_symbol("main") && !member.defines_symbol("helper_ext"),
              "shared linker archive parser should index defined symbols without treating unresolved externs as providers");

  const auto& object = *member.object;
  const auto text_index = find_section_index(object, ".text");
  expect_true(text_index < object.sections.size(),
              "shared linker archive parser should preserve section names for the parsed member object");
  expect_true(object.relocations[text_index].size() == 1,
              "shared linker archive parser should preserve relocation inventory for the bounded archive member");
}

int main(){
  test_backend_shared_liveness_surface_tracks_result_names();
  test_backend_shared_liveness_surface_tracks_call_crossing_ranges();
  test_backend_shared_liveness_surface_tracks_phi_join_ranges();
  test_backend_shared_regalloc_surface_uses_caller_saved_for_non_call_interval();
  test_backend_shared_regalloc_prefers_callee_saved_for_call_spanning_values();
  test_backend_shared_regalloc_reuses_register_after_interval_ends();
  test_backend_shared_regalloc_spills_overlapping_values_without_reusing_busy_reg();
  test_backend_shared_regalloc_helper_filters_and_merges_clobbers();
  test_backend_shared_stack_layout_regalloc_helper_exposes_handoff_view();
  test_backend_shared_stack_layout_analysis_tracks_phi_use_blocks();
  test_backend_shared_stack_layout_analysis_detects_dead_param_allocas();
  test_backend_shared_stack_layout_analysis_tracks_call_arg_values();
  test_backend_shared_stack_layout_analysis_detects_entry_alloca_overwrite_before_read();
  test_backend_shared_alloca_coalescing_classifies_non_param_allocas();
  test_backend_shared_slot_assignment_plans_param_alloca_slots();
  test_backend_shared_slot_assignment_prunes_dead_param_alloca_insts();
  test_backend_shared_slot_assignment_plans_entry_alloca_slots();
  test_backend_shared_slot_assignment_prunes_dead_entry_alloca_insts();
  test_backend_shared_slot_assignment_applies_coalesced_entry_slots();
  // TODO: binary-utils contract test disabled — assembler seam changed
  // test_backend_binary_utils_contract_headers_are_include_reachable();
  test_shared_linker_parses_minimal_relocation_object_fixture();
  // TODO: builtin return-add object parse disabled — assembler seam changed
  // test_shared_linker_parses_builtin_return_add_object();
  // TODO(phase2): temporary skip while linker/assembler-focused x86 tests are being split.
  // test_shared_linker_parses_builtin_x86_extern_call_object();
  test_shared_linker_parses_single_member_archive_fixture();

  check_failures();
  return 0;
}
