Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Source Plan: plan.md

# Active Item

- Step 2: switch one shared helper seam off `BackendModule`
- Current slice: extend the same BIR-first direct-call migration to the next
  production consumer, likely the aarch64 direct BIR emitter entry or the next
  shared declared-direct-call seam

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
- Added focused x86 BIR pipeline coverage for a direct BIR helper/main
  zero-argument direct-call module
- Switched `src/backend/x86/codegen/emit.cpp` to consume
  `parse_bir_minimal_direct_call_module(...)` before the affine-return BIR
  subset so direct BIR helper/main call pairs no longer require a legacy
  `BackendModule` parser on that path
- Rebuilt `backend_bir_tests` and reran the full
  `ctest --test-dir build -j --output-on-failure` suite successfully

# Next

- Extend the same migration pattern to the next direct-call consumer after x86,
  likely the aarch64 direct BIR entry or the next shared declared-direct-call
  seam
- Extend the BIR declared-direct-call surface with typed-call / extern-arg
  detail when a concrete emitter or shared helper needs it
- Continue shrinking stale `BackendModule`-only direct-call helpers one family
  at a time

# Notes

- Updated the stale `call_decode.hpp` comment to reflect that BIR already owns
  part of the direct-call surface, while legacy helpers still carry richer
  typed-call / extern-arg detail
- `test_before.log` was not present in the workspace at the start of this
  iteration; the last recorded full-suite baseline in `todo.md` was
  `2834/2834`, and this slice reran the full suite successfully in
  `test_after.log`
- This slice removes one real x86 direct-call consumer from the legacy
  `BackendModule` parser family, but aarch64 and declared-direct-call seams
  still remain
