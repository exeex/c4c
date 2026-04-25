Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reuse Shared Projection Helpers

Code Review Reminder Handled: review/address-projection-inventory-review.md found Step 1 inventory aligned and recommended continuing into Step 2.
Test Baseline Reminder Handled: accepted full-suite test_baseline.log for commit c97d5e69 after 3071 passed, 0 failed.

# Current Packet

## Just Finished

Step 3 continued shared projection-helper reuse in `addressing.cpp`.

Consolidated traversal:

- The non-first-index array/struct child traversal in
  `resolve_relative_global_gep_address` now consumes
  `resolve_aggregate_child_index_projection`.
- Relative first-index handling remains local to
  `resolve_relative_global_gep_address` and still applies whole-object layout
  scaling from the relative base address.
- Scalar leaf indexing policy remains local to the relative global path.
- The helper's `child_absolute_byte_offset` and `child_type_text` now provide
  the child offset and next type for the relative global GEP walk.

## Suggested Next

Continue Step 3 with a bounded dynamic aggregate traversal that still open-codes
array/struct child selection, while keeping dynamic-index and publication
policy local to the caller.

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
- `resolve_global_dynamic_pointer_array_access`,
  `resolve_local_aggregate_dynamic_pointer_array_access`, and dynamic aggregate
  GEP paths still open-code related traversal and were intentionally left
  outside this bounded packet.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
