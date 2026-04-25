Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Strengthen Helper Result Facts


Code Review Reminder Handled: review/address-projection-inventory-review.md found Step 1 inventory aligned and recommended continuing into Step 2.
Test Baseline Reminder Handled: no test_baseline.new.log exists for this todo-only metadata repair; no baseline was accepted.

# Current Packet

## Just Finished

Step 2 strengthened existing pure helper result facts without changing BIR
behavior.

Changed fields:

- `AggregateByteOffsetProjection` now carries `target_byte_offset`,
  `child_absolute_byte_offset`, and `child_stride_bytes` in addition to the
  existing child kind/index/type/layout facts.
- `ScalarLayoutByteOffsetFacts` now carries `target_byte_offset` and
  `remaining_object_bytes` in addition to object size and optional scalar leaf
  facts.

Consumers updated immediately:

- `find_repeated_aggregate_extent_at_offset_impl` now uses
  `child_stride_bytes` and `child_absolute_byte_offset` instead of recomputing
  those facts from child layout or parent fields.
- Follow-up correction: struct-field projections set `child_stride_bytes` from
  the child layout size, preserving the previous repeated-struct-field extent
  behavior while still exposing the fact through the helper result.
- `find_pointer_array_length_at_offset` now uses the projection result's
  `target_byte_offset` for the root-pointer-array check.
- Provenance scalar subobject containment now uses
  `remaining_object_bytes` from `resolve_scalar_layout_facts_at_byte_offset`
  while preserving the stored-type and opaque-pointer shortcuts.

## Suggested Next

Proceed to Step 3 with a bounded caller-consolidation packet. Start by routing
one duplicate aggregate traversal family through the existing helper vocabulary,
preferably in `addressing.cpp` before widening to local GEP call sites.

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
- `local_gep.cpp` was intentionally not touched in this packet because it was
  not in the owned file list; Step 3 should include it explicitly before moving
  local aggregate GEP traversal to shared helper facts.

## Proof

Ran the delegated proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.
