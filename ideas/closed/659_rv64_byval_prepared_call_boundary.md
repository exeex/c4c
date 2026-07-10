# RV64 Byval Prepared Call Boundary

Status: Closed
Type: Implementation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `ideas/open/669_byval_prepared_dump_contract_review.md`
- `ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md`
Owning Layer: RV64 byval/prepared call-boundary publication and consumption
Queue Order: 59
Proof Surface: current baseline rows 150, 151, 154, 155, 165, 207, 208, 209,
and 210 from `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.

## Goal

Repair the byval aggregate and prepared call-boundary path where prepared
formal/publication facts and RV64 call lowering disagree about aggregate,
pointer-argument, or formal GPR payload preservation.

## Why This Exists

Step 2 of the backend baseline umbrella classified nine current failed rows as
one broad first-owner family: dump, route, runtime, and object-runtime failures
around fixed byval aggregate calls, preserved pointer args, formal GPR
publication, and frame-slot pointer-arg payload preservation. This breadth
should be handled before narrow singleton runtime rows.

## In Scope

- Refresh focused dump, route, object, and runtime evidence for the nine byval
  and prepared call-boundary rows.
- Identify whether the first broken boundary is prepared call-boundary
  publication, byval aggregate source/destination copying, formal GPR
  publication, or RV64 consumption of already-published facts.
- Repair a general byval/prepared call-boundary rule only when explicit
  prepared facts prove source payload, destination home, and call ABI
  ownership.
- Add or update focused backend proof only for the selected byval or prepared
  call-boundary owner.

## Out Of Scope

- RV64 pointer-local postincrement or Duff pointer-update lowering.
- Prepared stack-destination fan-in authority from ideas 647 or 655.
- The `loop-2e.c` representative writeback route from idea 657.
- AArch64 instruction dispatch, CLI formatting, LLVM torture owner discovery,
  or unrelated object-data static-storage policy.
- Test expectation rewrites, unsupported-marker changes, allowlist edits,
  timeout changes, runtime policy changes, or baseline acceptance changes.

## Acceptance Criteria

- Focused evidence names the first byval/prepared call-boundary owner for the
  nine current failed rows or records a smaller split with concrete proof.
- The selected owner publishes or consumes explicit prepared call-boundary
  facts instead of relying on testcase names or final assembly shape.
- The repaired route makes the focused byval/prepared call-boundary subset
  pass or fails closed with precise diagnostics at the proven owner.
- Backend regression proof shows no new backend failures in the supervisor's
  chosen subset.

## Completion Notes

Closed after the selected byval prepared call-boundary owner was repaired and
proved through route/runtime coverage. The focused Step 3 proof recorded 6/9
rows passing: the three codegen-route rows and the three RV64 runtime rows for
byval aggregate, preserved pointer args, and formal GPR publication.

The remaining focused rows were split rather than absorbed into this completed
runtime/codegen route:

- `backend_dump_riscv64_byval_aggregate_fixed_call` and
  `backend_dump_riscv64_byval_preserved_pointer_args` now belong to
  `ideas/open/669_byval_prepared_dump_contract_review.md`.
- `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` now
  belongs to
  `ideas/open/670_byval_frame_slot_object_runtime_binaryinst.md`.

Close-gate regression proof used the same-scope backend guard:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Before `passed=343 failed=25 total=368`; after `passed=349
failed=19 total=368`; resolved 6 backend rows; new failing tests: 0.

## Reviewer Reject Signals

- Reject named-case fixes for the byval aggregate, preserved pointer arg,
  formal GPR, or frame-slot pointer-arg tests.
- Reject downgrading any byval/prepared call-boundary test to unsupported or a
  weaker contract without explicit user approval.
- Reject claiming progress through expectation rewrites, helper renames,
  classification-only edits, or baseline accounting changes.
- Reject merging RV64 pointer-local lowering, stack fan-in authority, AArch64
  dispatch, CLI dump formatting, or LLVM torture diagnosis into this route.
- Reject a patch that leaves the same byval payload or formal-publication
  failure behind a new abstraction name.
