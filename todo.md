Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reuse Shared Projection Helpers

Code Review Reminder Handled: review/address-projection-inventory-review.md found Step 1 inventory aligned and recommended continuing into Step 2.
Test Baseline Reminder Handled: accepted full-suite test_baseline.log for commit c97d5e69 after 3071 passed, 0 failed.

# Current Packet

## Just Finished

Step 3 continued shared projection-helper reuse in `local_gep.cpp`.

Consolidated traversal:

- The constant-index array/struct child traversal before a local dynamic
  pointer-array index in `resolve_local_aggregate_dynamic_pointer_array_access`
  now consumes `resolve_aggregate_child_index_projection` while preserving the
  old zero-sized array-element rejection.
- The constant-index tail walk after the dynamic array index also consumes
  `resolve_aggregate_child_index_projection`.
- The first-index policy, dynamic-index lowering, pointer-leaf return policy,
  and local slot publication policy remain local to the caller path.
- The dynamic tail walk keeps its existing layout validity guards before using
  the helper for child offset and next-type projection.

## Suggested Next

Continue Step 3 by applying the shared projection helper to another bounded
constant-index aggregate traversal, or ask the reviewer to check whether the
remaining helper-reuse opportunities are still within this runbook step.

## Watchouts

- Preserve BIR output, diagnostics, and testcase expectations.
- Keep `BirFunctionLowerer` as the stateful lowering owner.
- Prefer existing `memory_helpers.hpp`; do not add a new header unless the
  active plan is explicitly revised.
- Do not turn the inventory into testcase-shaped matching.
- Do not move publication policy into pure helpers. Helpers should report facts;
  callers should still decide which state map to update.
- Be careful with first-index policy: local aggregate GEP, absolute global GEP,
  relative global GEP, dynamic aggregate GEP, and runtime pointer provenance use
  similar layout math but different acceptance rules.
- Dynamic aggregate GEP paths still open-code related traversal and remain
  candidates for separate bounded packets.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
