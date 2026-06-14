Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fail-Closed Behavior Matrix

# Current Packet

## Just Finished

Step 3 - Fail-Closed Behavior Matrix defined required behavior for each
candidate or ambiguous `PreparedBirModule::liveness` ownership row before any
future one-reader candidate could be trusted.

Fail-closed matrix:

- Identity-only liveness tests
  (`tests/backend/bir/backend_prepare_liveness_test.cpp`): absent liveness must
  fail assertions that expect the function/value/block/edge identity record;
  invalid IDs or stale names must fail lookup-by-name or ID-spelling checks;
  mismatched function membership must not be silently matched to another
  function; fallback must not synthesize a policy record from names alone;
  duplicate/conflicting value or block records need explicit rejection or test
  coverage before becoming a candidate; unsupported liveness facts must remain
  observable as missing identity evidence; policy-sensitive interval,
  cross-call, loop-depth, and call-point facts are blockers, not identity-only
  metadata.
- Prepared filtering
  (`src/backend/backend.cpp::filter_prepared_module_to_function(...)`):
  absent/skipped liveness must preserve an empty or missing compatibility
  surface without inventing rows; invalid or stale function IDs must not retain
  unrelated `filtered.liveness.functions`; mismatches between filtered module
  functions and liveness functions must fail closed by erasing non-selected
  rows; fallback must preserve the existing dump/filtering contract rather than
  hide liveness in a private pass context; duplicate/conflict behavior is
  unproven and blocks a one-reader candidate; unsupported rows must stay a
  status/debug compatibility concern; policy-sensitive derived rows must remain
  out of filtering.
- Construction-only liveness
  (`src/backend/prealloc/liveness.cpp::BirPreAlloc::run_liveness()` and
  `PrepareOptions::run_liveness`): absent/skipped liveness is unresolved
  normal-path ambiguity because tests toggle the option and sometimes call
  `run_liveness()` manually, while the normal pipeline currently populates
  liveness before regalloc. A future candidate must prove that skipped,
  cleared, or stale liveness cannot feed regalloc, call planning, helper
  planning, printer/status, or target behavior. Invalid, stale, mismatch,
  fallback, duplicate/conflict, unsupported, and policy-sensitive states are
  blockers until they have explicit fail-closed tests.
- Regalloc/value-location authority
  (`src/backend/prealloc/regalloc.cpp::BirPreAlloc::run_regalloc()` plus
  `regalloc/intervals.cpp`, `classification.cpp`, `spill_reload.cpp`, and move
  helpers): absent liveness currently yields no seeded regalloc/value-location
  rows because iteration is over `prepared_.liveness.functions`; that is
  fail-closed only if all downstream consumers reject missing rows. Invalid or
  stale value IDs, mismatched function names, duplicate intervals, conflicting
  register class/group width, unsupported value kinds, fallback stack-slot
  allocation, and policy-sensitive `crosses_call` facts all affect allocation,
  target register, storage/home, and move scheduling. These rows are demotion
  blockers, not candidates.
- Call plans
  (`src/backend/prealloc/call_plans.cpp::populate_call_plans(...)` and
  `build_call_preserved_values(...)`): absent liveness or a missing call
  program point returns no preserved values, which is fail-closed only for
  preservation-route publication and not proof that liveness is optional.
  Invalid/stale function IDs, mismatched call points, duplicate/conflicting
  preserved values, unsupported homes, and fallback route selection must not
  synthesize preservation routes. Policy-sensitive stack-slot versus
  callee-saved-register endpoints remain blockers.
- i128 runtime helpers
  (`src/backend/prealloc/i128_runtime_helpers.cpp`): absent liveness, missing
  regalloc, or missing helper call point records
  `live_preservation_requires_structured_live_across_helper_facts`; incomplete
  preserved routes record
  `live_preservation_requires_complete_preserved_value_routes`; selected call
  ownership records
  `selected_call_ownership_requires_live_preservation_policy` unless complete
  live preservation is available. Invalid, stale, mismatch, duplicate/conflict,
  unsupported, fallback, and policy-sensitive states must continue to block
  `owns_terminal_call`.
- f128 runtime helpers
  (`src/backend/prealloc/f128_runtime_helpers.cpp`): required behavior mirrors
  i128. Absent or incomplete liveness-derived facts must keep live-preservation
  policy incomplete and selected call ownership false. Existing AArch64 f128
  code also compares prepared helper policy with target record policy, so stale
  or mismatched helper records must fail closed instead of reusing old policy.
- Prepared printer/status
  (`src/backend/prealloc/prepared_printer/runtime_helpers.cpp`): absent or
  incomplete liveness-derived helper policy must print missing/incomplete
  live-preservation rows, not omit the status surface. Invalid/stale/mismatch,
  duplicate/conflict, unsupported, fallback, and policy-sensitive rows are
  output/status compatibility blockers because printer output depends on
  helper live-preservation facts even without a direct liveness field read.
- AArch64 target behavior
  (`src/backend/mir/aarch64/codegen/i128_ops.cpp`,
  `src/backend/mir/aarch64/codegen/f128.cpp`, and target/printer tests):
  absent or incomplete live-preservation policy must block helper lowering or
  remain diagnostic/status-only. Invalid, stale, mismatch, duplicate/conflict,
  unsupported, fallback, and policy-sensitive target register/storage states
  must not produce machine records that assume preservation. These are derived
  target-policy blockers.
- x86 prepared consumers: absent or incomplete liveness-derived regalloc,
  `value_locations`, or call-plan facts must remain explicit unsupported or
  missing-home behavior. Invalid/stale/mismatched homes, duplicate/conflicting
  publications, unsupported home kinds, and fallback storage routes must not
  become guessed moves. This remains derived target/storage compatibility, not
  a direct liveness reader.
- RISC-V aggregate risk: no direct liveness, interval, or live-preservation
  read was found, but absent or changed prepared aggregate shape must not break
  RISC-V prepared edge-publication behavior. Invalid/stale/mismatch,
  duplicate/conflict, unsupported, fallback, and policy-sensitive rows are
  proof gaps at the aggregate boundary, not permission to hide liveness.

Existing proof rows found:

- `backend_prepare_liveness_test.cpp` proves identity publication, phi
  predecessor edge identity, regalloc projection from liveness, loop-weighted
  priority, cross-call boundary facts, helper-family liveness, and printed
  live-preservation rows.
- `backend_prepare_frame_stack_call_contract_test.cpp` proves call-point
  publication for direct, float, indirect, dynamic-stack, and cross-call
  preservation contracts.
- i128 helper tests in `backend_prepare_liveness_test.cpp` prove missing
  structured live-across-helper facts, incomplete preserved routes, and
  selected-call ownership requiring live-preservation policy.
- AArch64 tests in
  `backend_aarch64_target_instruction_records_test.cpp` and
  `backend_aarch64_machine_printer_test.cpp` prove derived helper
  live-preservation policy and selected-call ownership behavior.
- x86 and RISC-V prepared edge-publication tests prove some unsupported or
  missing-home behavior for derived value-location consumers, but not direct
  liveness absence or stale-liveness states.

Proof gaps and blockers:

- No complete fail-closed test was found for `PrepareOptions::run_liveness =
  false` feeding normal regalloc/call-plan/helper/printer paths; absent or
  skipped liveness remains an explicit blocker.
- No direct test was found for stale liveness after module/function/value
  mutation, mismatched function IDs, duplicate liveness function rows, or
  conflicting duplicate value IDs.
- Regalloc fallback-to-stack behavior is tested as allocation behavior, but not
  as a stale/invalid liveness rejection contract.
- Call-plan preserved-value fallback is fail-closed for missing liveness and
  missing call points, but route conflicts, duplicate preserved values, and
  stale call-point mappings still need explicit proof before any demotion.
- i128/f128 helper missing-fact rows prove some absent/incomplete behavior, but
  invalid IDs, stale helper call points, and duplicate/conflicting preserved
  routes remain proof gaps.
- Prepared printer, AArch64, x86, and RISC-V rows are derived compatibility
  surfaces; their current tests do not prove that liveness can be hidden from
  prepared public state.

## Suggested Next

Execute Step 4 by evaluating the identity-only rows against this matrix and
deciding whether any one-reader candidate remains. Treat unresolved
`PrepareOptions::run_liveness`, duplicate/conflict, stale, and derived
target/printer proof gaps as blockers unless the row excludes them completely.

## Watchouts

- This is analysis-only; do not edit implementation code or expectations under
  this active plan.
- `PrepareOptions::run_liveness` remains unresolved and must be treated as an
  absent/skipped liveness blocker for normal-path behavior.
- Filtering and construction-only rows have identity-only pieces, but both are
  compatibility-bound by public prepared liveness and cannot justify demotion
  while regalloc, call-plan, and helper-planning consumers remain direct
  readers.
- Direct-field consumers are fewer than derived consumers. Keep printer,
  AArch64, x86, and RISC-V aggregate behavior distinct when deciding Step 4.

## Proof

`git diff --check -- todo.md`

Passed for this packet. The delegated proof command does not produce
`test_after.log`; no replacement log was created.
