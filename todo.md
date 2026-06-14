Status: Active
Source Idea Path: ideas/open/256_phase_f3_prepared_liveness_authority_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: One-Reader Candidate Decision

# Current Packet

## Just Finished

Step 4 - One-Reader Candidate Decision evaluated the Step 2 identity-only rows
against the Step 3 fail-closed matrix.

Decision: no one-reader implementation candidate remains. `PreparedBirModule::liveness`
remains blocked public prepared planning authority because every identity-shaped
row either depends on retained public prepared compatibility or is attached to
liveness records that still feed allocation, storage, helper planning, target
register/storage policy, output/status policy, or unproven stale,
duplicate/conflict, and mismatch behavior.

Rejected identity-only rows:

- `tests/backend/bir/backend_prepare_liveness_test.cpp` direct identity checks
  are not a reader candidate. The tests assert the public prepared liveness
  surface itself: function membership, value IDs, block labels, CFG edge IDs,
  phi predecessor edges, call points, intervals, loop depth, and cross-call
  facts. The identity-only subset cannot be split from policy-sensitive fields
  because the same public records are consumed by regalloc, call plans, i128/f128
  helper planning, printer/status, and target-derived behavior. Step 3 also
  found no complete stale, mismatch, duplicate/conflict, or unsupported-row proof
  that would let an implementation hide the field while preserving only identity
  assertions.
- `src/backend/backend.cpp::filter_prepared_module_to_function(...)` is not a
  reader candidate. It uses function identity only to retain or erase
  `filtered.liveness.functions`, but that retained aggregate is the prepared
  dump/filtering compatibility surface. A future one-reader packet cannot use
  filtering as the sole reader unless it also preserves the public liveness
  compatibility record and proves absent/skipped, stale, mismatch, duplicate,
  conflict, and unsupported behavior. Those proof rows are missing, and the
  retained record still needs to remain aligned with regalloc, call-plan,
  helper, printer, AArch64, x86, and RISC-V aggregate compatibility.
- `src/backend/prealloc/liveness.cpp::BirPreAlloc::run_liveness()` is not a
  reader candidate. It is a producer/construction step, not a consumer to demote.
  The produced records include identity plus intervals, program points, call
  points, loop depth, predecessor-edge facts, and `crosses_call` data. Those
  facts feed `BirPreAlloc::run_regalloc()`, `populate_call_plans(...)`, i128/f128
  runtime-helper planning, and derived target/printer behavior.
  `PrepareOptions::run_liveness` is also unresolved: skipped or manually-run
  liveness is not proven fail-closed for normal regalloc, call-plan,
  helper-planning, printer, or target paths.

Rejected ambiguous/direct policy rows:

- `BirPreAlloc::run_regalloc()` and regalloc interval/classification/spill-reload
  helpers are allocation, target-register, storage/home, and move-scheduling
  authority. Their use of liveness cannot be narrowed to identity.
- `populate_call_plans(...)` and `build_call_preserved_values(...)` are
  carrier/helper plus target-storage authority. Missing liveness may suppress
  preserved values, but route conflicts, stale call points, and duplicate
  preserved values remain unproven.
- i128/f128 runtime-helper planners are helper-planning, live-preservation,
  selected-call ownership, and status authority. Their missing-fact rows are
  compatibility behavior, not permission to hide or demote liveness.
- Prepared printer/status, AArch64, x86, and RISC-V rows are derived
  compatibility blockers. They are not direct one-reader candidates because they
  depend on liveness-derived helper, regalloc, value-location, call-plan, or
  aggregate-shape contracts.

Required future state before any separate implementation idea:

- a single exact reader that consumes one semantic fact without allocation,
  storage, helper planning, target register/storage, move scheduling, or output
  authority;
- a retained prepared compatibility surface for public dump/filter/status tests;
- explicit fail-closed proof for absent/skipped liveness, stale records,
  function/value mismatch, duplicate/conflict rows, unsupported facts, fallback
  paths, and derived printer/target compatibility.

No current row satisfies that contract, so no separate implementation source
idea should be opened from Step 4.

## Suggested Next

Execute Step 5 by summarizing the liveness consumer buckets, the rejected
one-reader candidates, the remaining fail-closed proof gaps, and the final
readiness decision. Keep the runbook analysis-only and do not request an
implementation split from this evidence.

## Watchouts

- This is analysis-only; do not edit implementation code or expectations under
  this active plan.
- The no-candidate decision depends on missing fail-closed evidence as well as
  direct policy consumers; a future idea must add proof before changing field
  ownership.
- Do not treat liveness identity tests, filtering, or construction as an
  implementation target unless a later source idea names one exact reader and
  supplies the full fail-closed proof set.
- `PrepareOptions::run_liveness` remains a normal-path absent/skipped liveness
  ambiguity and should stay in the final blocker summary.

## Proof

`git diff --check -- todo.md`

Passed for this packet. The delegated proof command does not produce
`test_after.log`; no replacement log was created.
