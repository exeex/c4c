Status: Active
Source Idea Path: ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select and Map One Comparison Consumer

# Current Packet

## Just Finished

Step 1 mapping selected exactly one bounded consumer:
`aarch64::codegen::lower_materialized_compare_condition_branch(...)` in
`src/backend/mir/aarch64/codegen/comparison.cpp`.

Selected consumer and bound:
- Consumer: materialized compare condition branch lowering for a conditional
  terminator whose prepared branch condition names the BIR terminator condition.
- Why bounded: it is one branch consumer, uses one condition value from the
  current BIR block, and leaves branch suffix selection, scratch-register
  choice, publication emission, and final machine instruction formation in
  AArch64 lowering.
- Current non-Route-7 lookup: `bir::find_materialized_condition_producer_identity(
  *context.bir_block, branch_condition.condition_value,
  context.bir_block->insts.size())`.
- Route 7 key/facade to use next: build
  `bir::route7_build_comparison_condition_index(*context.bir_block)` and query
  `bir::route7_find_materialized_condition(index, *context.bir_block,
  branch_condition.condition_value, context.bir_block->insts.size())`;
  validation can use
  `bir::route_index_validate_materialized_condition_reference(
  bir::route_index_reference_facade(index), *context.bir_block,
  branch_condition.condition_value, context.bir_block->insts.size())`.
- Existing fallback path: if the Route 7 materialized-condition record is
  absent, invalid, stale, duplicate, or not an available comparison, keep the
  existing BIR helper path first; if that still cannot produce a binary compare,
  lowering naturally falls back to the later materialized-bool branch path
  `lower_conditional_branch_from_emitted_condition(...)`.
- Relevant existing tests: `backend_prepared_lookup_helper` covers Route 7
  materialized-condition records, facade validation, missing/duplicate/stale
  rejection, and prepared/BIR oracle parity; `backend_aarch64_branch_control_lowering`
  covers the selected materialized compare branch lowering and materialized-bool
  fallback behavior; `backend_aarch64_instruction_dispatch` covers prepared
  branch-condition publication fallback used when the condition value must be
  materialized.

## Suggested Next

Delegate Step 2 to an executor: thread a route-first materialized-condition
lookup into `lower_materialized_compare_condition_branch(...)`, using the Route
7 key above and preserving the existing BIR/helper and emitted-condition
fallbacks for absent or invalid Route 7 data.

## Watchouts

- Keep return-chain lookup outside this migration.
- Do not move target branch policy or ALU machine-record formation into BIR.
- Do not weaken tests or remove invalid-reference, absent-route, or fallback
  cases.
- Do not claim route-wide comparison helper contraction from one selected
  consumer.
- Do not reuse the already-migrated fused-compare selected-operand consumer as
  the Step 2 target; this packet selected the materialized-condition branch
  consumer only.

## Proof

Mapping-only packet. Per delegation, no build/tests were run and no
`test_after.log` was produced.

Recommended narrow proof command for the next code-changing packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'`
