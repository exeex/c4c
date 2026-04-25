Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reuse Shared Projection Helpers


Code Review Reminder Handled: review/address-projection-inventory-review.md found Step 1 inventory aligned and recommended continuing into Step 2.
Test Baseline Reminder Handled: no test_baseline.new.log exists for this todo-only metadata repair; no baseline was accepted.

# Current Packet

## Just Finished

Step 3 continued shared projection-helper reuse in `local_gep.cpp`.

Consolidated traversal:

- The constant non-first-index array/struct child traversal in
  `resolve_local_aggregate_pointer_array_slots` now consumes
  `resolve_aggregate_child_index_projection` and uses the helper's
  `child_absolute_byte_offset`, `child_type_text`, `child_layout`, and parent
  array count facts.
- The final pointer-array slot collection remains local to
  `resolve_local_aggregate_pointer_array_slots`.
- First-index reinterpret, repeated-extent policy, and the scalar-array
  early collection branch remain in the local call site.
- Publication and local-state policy were not moved.

## Suggested Next

Continue Step 3 with another bounded local or dynamic aggregate traversal that
still open-codes array/struct child selection, while keeping dynamic-index and
publication policy local to the caller.

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
- `resolve_local_aggregate_dynamic_pointer_array_access` and dynamic aggregate
  GEP paths still open-code related traversal and were intentionally left
  outside this bounded packet.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
