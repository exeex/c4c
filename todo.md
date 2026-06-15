Status: Active
Source Idea Path: ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Locate the Identity-Only Reader

# Current Packet

## Just Finished

Step 2, `Locate the Identity-Only Reader`, classified the Step 1 reader set
without implementation changes. No exact identity-only reader exists today.

Reader classification:
- `BirPreAlloc::run_regalloc()` in `src/backend/prealloc/regalloc.cpp:470`
  is `target-policy`, not identity-only. It sizes derived regalloc/value-location
  output from `prepared_.liveness.functions` at lines 473 and 475, reads every
  `PreparedLivenessFunction` at lines 479 and 485, then combines value identity
  with target and allocation policy: register class and group width at lines
  500-518, published regalloc value identity plus type/kind/home/call-crossing
  facts at lines 520-540, allocation constraints and caller/callee-saved policy
  at lines 546-568, interference from live intervals at lines 578-596, active
  register assignment and dynamic-stack/call-point policy at lines 635-735,
  stack-slot fallback allocation at lines 737-775, and spill/reload publication
  at line 779. The identities consumed include `function_name`, `value_id`,
  `stack_object_id`, and `value_name`, but they are consumed with live intervals,
  use points, call points, type/kind, target register profile, stack layout, and
  spill policy.
- Regalloc helpers in `src/backend/prealloc/regalloc/intervals.cpp` are
  `target-policy` support. `value_priority()` at line 13 uses use-point count,
  live interval span, `crosses_call`, and `requires_home_slot`; `locate_program_point()`
  at line 46 maps program points to block/instruction locations; and
  `weighted_use_score()` at line 61 weights uses by loop depth. These helpers
  consume semantic liveness timing data for allocation priority, not only identity.
- `classify_register_class()` and override resolution in
  `src/backend/prealloc/regalloc/classification.cpp:9` are `target-policy`.
  They classify `PreparedLivenessValue::type` and prepared override annotations
  into register classes/group widths, so the liveness value identity is only a
  key into downstream target allocation policy.
- `append_spill_reload_ops()` in
  `src/backend/prealloc/regalloc/spill_reload.cpp:13` is `target-policy`/derived
  output. It maps spill points and value use points back through the liveness
  function to publish spill/reload ops with register bank, register names,
  contiguous widths, occupied registers, slot ids, and stack offsets.
- `populate_call_plans()` in `src/backend/prealloc/call_plans.cpp:2751` and
  `build_call_preserved_values()` at lines 1479 and 1588 are `target-policy`
  call-preservation readers. They locate the liveness row by `function_name`,
  map block/instruction to call program points, and combine live intervals with
  regalloc candidates, frame plans, value homes, target register placement, stack
  slots, callee-saved save indices, preservation endpoints, and preservation
  reasons. The identity consumed is `function_name` plus value ids/names, but
  the behavior is preservation-route publication rather than identity-only use.
- `populate_f128_runtime_helper_call_ownership()` in
  `src/backend/prealloc/f128_runtime_helpers.cpp:917` and
  `populate_f128_runtime_helper_facts()` at line 1164 are `target-policy`/status
  helper readers. They find the helper call point in liveness at lines 926-931,
  build preserved values at lines 932-940, and publish missing-fact status such
  as `live_preservation_requires_structured_live_across_helper_facts` at lines
  946-950 when liveness/regalloc/call-point evidence is absent. The identity is
  `function_name` and helper block/instruction location, consumed to publish
  helper ownership and status.
- `populate_i128_runtime_helper_call_ownership()` in
  `src/backend/prealloc/i128_runtime_helpers.cpp:935` and
  `populate_i128_runtime_helper_lanes()` at line 1441 are `target-policy`/status
  helper readers for the same reason: call-point lookup at lines 947-952,
  preserved-value construction at lines 953-961, and helper policy/status
  publication. Their identities are function identity and helper call location,
  not standalone liveness identity.
- Route-debug focus filtering in `src/backend/backend.cpp:588` is
  `printer/debug/status` compatibility. Missing function focus clears
  `filtered.liveness.functions` at line 599, while function focus erases rows
  outside the selected `FunctionNameId` at lines 635-639. This is a structured
  debug/filtering compatibility surface, not a semantic identity consumer.
- Prepared printing in `src/backend/prealloc/prepared_printer.cpp:39` is
  `printer/debug/status` compatibility. It does not print raw liveness rows;
  instead `print()` emits completed phases, notes, BIR body, and liveness-derived
  summaries such as value locations, call plans, regalloc, storage plans, and
  helper facts via appenders beginning at line 77.
- Target emission paths identified in Step 1 are `target-policy` derived
  consumers. They consume regalloc, value locations, call plans, stack layout,
  storage/helper facts, and exact target output derived after liveness, not raw
  `PreparedLiveness`.
- Tests and support helpers in `tests/backend/bir/backend_prepare_liveness_test.cpp:91`,
  `tests/backend/bir/backend_prepare_liveness_test.cpp:113`, and
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:100`
  are `compatibility/test support`. They intentionally read the public prepared
  field to assert current behavior and cannot authorize private pass context or
  demotion.
- No `fallback`, `wrapper`, or `unsupported behavior` reader was found in the
  Step 1 direct-reader set. Fallback/unsupported behavior remains unmapped until
  Step 3 fail-closed rows.

Decision:
- Candidate identity-only readers: none accepted. Every production reader that
  touches prepared liveness consumes either target allocation policy, call
  preservation policy, helper ownership/status policy, route-debug/filtering
  compatibility, or test/support compatibility in addition to identity.
- Candidate semantic liveness fact: still only the prepared liveness dataflow
  row produced by `BirPreAlloc::run_liveness()` in
  `src/backend/prealloc/liveness.cpp:906`, with `PreparedLivenessFunction`
  publication at line 1001. Step 1 found no separate BIR or route-level
  liveness fact that owns semantic meaning independently.
- Therefore one exact identity-only reader and one independent semantic
  liveness fact do not exist today. The only semantic fact is the public
  prepared row itself, and the direct readers either derive target-facing
  behavior from it or preserve public compatibility around it.
- Because the only semantic authority is also the public prepared field, and no
  exact identity-only reader proves a narrow private-pass-context boundary,
  private-pass-context, demotion, wrapper, migration, or deletion work remains
  unsafe. Any such packet would risk moving or hiding the only current semantic
  authority while production readers still rely on liveness-derived allocation,
  preservation, helper status, route-debug, and test compatibility behavior.

## Suggested Next

Execute Step 3 from `plan.md`: map fail-closed rows for absent/skipped,
stale, mismatch, duplicate/conflict, unsupported, fallback, and derived
printer/target behavior.

## Watchouts

- This active plan is analysis and blocker mapping only.
- Do not demote, delete, privatize, wrap, migrate, or implement
  `PreparedBirModule::liveness`.
- Do not weaken printer, route-debug, helper/oracle/status, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  classification-only notes, or named-fixture proof.
- Step 2 did not find an exact identity-only reader. Treat public
  `PreparedBirModule::liveness` as blocked semantic authority until Step 3
  fail-closed evidence is mapped and the supervisor/plan-owner decide whether a
  separate lifecycle idea is needed.
- `PrepareOptions::run_liveness` appears present but not honored by the active
  `BirPreAlloc::run()` pipeline; treat absent/skipped row behavior as unmapped
  until Step 3.

## Proof

Analysis-only packet. Delegated proof command:
- read back `todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required by the packet because only `todo.md` changed.
