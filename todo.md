Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 2: switch one shared helper seam off `BackendModule`
- Current slice: finish the first BIR-side direct-call parser contract in
  `src/backend/lowering/call_decode.hpp` so the helper views match real
  direct-call shapes instead of stale placeholder semantics

# Completed

- Read `plan.md` and the linked source idea
- Confirmed `todo.md` was missing and initialized execution state for this run
- Inspected `call_decode.hpp` and found existing BIR helpers for minimal direct
  call and declared direct call module parsing
- Recorded the full-suite baseline in `test_before.log`
- Fixed `parse_bir_minimal_direct_call_module(...)` so it matches a helper
  definition returning an immediate instead of a declaration-only placeholder
- Extended `parse_bir_minimal_declared_direct_call_module(...)` to preserve
  whether the caller returns the call result or a fixed immediate
- Added focused shared-util coverage for both BIR direct-call parser helpers
- Rebuilt from scratch and confirmed full-suite parity:
  `2834/2834` tests passed before and after

# Next

- Wire one real consumer to use the corrected BIR direct-call helper view
- Extend the BIR declared-direct-call surface with typed-call / extern-arg
  detail when a concrete emitter or shared helper needs it
- Continue shrinking stale `BackendModule`-only direct-call helpers one family
  at a time

# Notes

- Updated the stale `call_decode.hpp` comment to reflect that BIR already owns
  part of the direct-call surface, while legacy helpers still carry richer
  typed-call / extern-arg detail
- This slice improves the BIR contract and test coverage, but it does not yet
  switch an emitter or shared production helper off the legacy parser family
