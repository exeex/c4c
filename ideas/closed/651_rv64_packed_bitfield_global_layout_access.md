# RV64 Packed Bitfield Global Layout And Access Lowering

Status: Closed
Type: Implementation
Parent: `ideas/closed/642_rv64_global_residual_runtime_mismatch_research.md`
Related:
- `ideas/closed/642_rv64_global_residual_runtime_mismatch_research.md`
- `docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md`
Owning Layer: RV64 packed bitfield global-object layout and access lowering
Queue Order: 51
Proof Surface: `src/pr79737-2.c` and focused packed bitfield/global-object unit coverage

## Goal

Repair RV64 global-object materialization for packed bitfield aggregates whose
semantic storage size is not a whole-word lane shape, starting with the
72-bit packed file-scope struct used by `src/pr79737-2.c`.

## Why This Exists

Idea 642 proved that `src/pr79737-2.c` is not a true runtime-support failure.
C4C currently materializes the packed file-scope globals `i` and `j` as
12-byte word-lane objects, while the clang control path lays the same packed
bitfield aggregate out as 9 bytes and uses byte-oriented accesses. The binary
then reaches the source predicate and aborts because the generated bitfield
values differ.

## In Scope

- Preserve packed bitfield aggregate size for file-scope globals when layout
  authority proves a 9-byte object rather than a 12-byte word-lane object.
- Lower global bitfield loads and stores for the selected packed aggregate
  through byte-lane access or an equivalent semantic lowering that preserves
  the packed representation.
- Add focused positive and fail-closed coverage for packed bitfield global
  layout and access lowering.
- Rerun `src/pr79737-2.c` through the RV64 GCC C torture backend harness after
  the semantic repair.

## Out Of Scope

- Generic runtime-support changes.
- Reopening direct global-symbol local-memory admission from idea 631.
- Broad ABI, call-lowering, branch/control-flow, relocation, or stack-layout
  rewrites.
- Named-case shortcuts for `src/pr79737-2.c`.
- Expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or
  pass/fail accounting changes.

## Acceptance Criteria

- C4C lays out both `i` and `j` from `src/pr79737-2.c` as 9-byte packed
  file-scope globals, matching the proven control layout from idea 642.
- Generated RV64 global accesses preserve the packed bitfield representation
  instead of using the old 12-byte word-lane model.
- `src/pr79737-2.c` advances under the RV64 GCC C torture backend harness
  without weakening the harness or expectations.
- Focused unit coverage rejects unsupported or incomplete authority shapes
  fail-closed rather than silently choosing word-lane storage.

## Closure Notes

Closed after Step 7 broader validation. The representative
`tests/c/external/gcc_torture/src/pr79737-2.c` now has HIR `struct S size=9
align=1`, LIR/LLVM `%struct.S = type <{ [9 x i8] }>`, 9-byte ELF `OBJECT
GLOBAL` symbols for both `i` and `j`, and prepared byte-storage aggregate
global accesses at offsets 0, 2, and 5 with `range_verdict=proven_in_bounds`.

Focused packed-global coverage passed, including fail-closed zero-width and
too-small packed bitfield cases plus the representative RV64 GCC torture
backend route. Broader matched regression guard passed with `test_before.log`
and `test_after.log`: before `366/398` passed with 32 known failures, after
`369/401` passed with the same 32 known failures, `+3` passes, and no new
failures.

## Reviewer Reject Signals

- Reject a fix that special-cases `src/pr79737-2.c`, the names `i` or `j`, or
  the exact test filename instead of repairing packed bitfield global layout
  and access semantics.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime
  comparison, or pass/fail accounting edits claimed as capability progress.
- Reject a route that only changes symbol size reporting while global loads or
  stores still consume a 12-byte word-lane representation.
- Reject broad runtime-support, ABI, relocation, branch, or stack rewrites that
  do not prove the packed bitfield global-object owner first.
- Reject helper renames, classification-only changes, or abstraction moves that
  leave the 9-byte packed global versus 12-byte C4C global mismatch intact.
