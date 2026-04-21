# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step 2.1 inspected the reduced `match` helper route behind
`c_testsuite_x86_backend_src_00204_c` and stopped without a repair. The
remaining idea-60 blocker is not the old prepared short-circuit handoff:
`match` now reduces to a multi-block pointer-backed loop/return route whose
join/return leaves already fit the existing prepared compare-join family, but
the x86 path still lacks a generic way to consume the pointer-induction updates
that drive the loop (`f++`, `p++`, and the final `*s = p - 1` store).

## Suggested Next

Build the next idea-60 packet around generic pointer-valued induction and
pointer-store consumption in `render_prepared_local_slot_guard_chain_if_supported`
or the adjacent shared helpers, then add the nearest backend handoff coverage
for a pointer-backed short-circuit loop/return route once that generic pointer
update seam is renderable.

## Watchouts

- Do not special-case `match` or one named short-circuit loop shape. The route
  already proves that prepared compare-join return metadata and pointer-backed
  global leaves exist; the missing seam is generic pointer induction/rendering.
- `render_prepared_compare_driven_entry_if_supported` still routes only through
  the one-parameter compare-against-zero family, so avoid masking the real
  local-slot pointer-update gap by bolting on another compare-entry matcher.
- Keep idea-61 ownership closed unless a future reduced probe falls back into
  prepared-module or call-bundle rejection before scalar emission.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the idea-60 scalar
restriction. Supporting investigation also confirmed that `match` lowers to a
pointer-backed loop/return BIR route and that current x86 local-slot rendering
has pointer-home sync/load/store helpers but no generic pointer arithmetic lane
for the induction updates that keep the loop moving. Canonical proof log:
`test_after.log`.
