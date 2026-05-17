# Prealloc Regalloc Implementation Decomposition

Status: Closed
Created: 2026-05-17
Closed: 2026-05-17

## Intent

Split `src/backend/prealloc/regalloc.cpp` into focused implementation owners
while preserving register allocation behavior.

The current file is not only a register allocator. It also discovers special
values, maps runtime helpers, publishes value locations, creates call/return
move resolutions, tracks pointer carriers, and emits spill/reload operations.

## Why This Exists

AArch64 bring-up required many prepared facts near allocation decisions, so
`regalloc.cpp` became the place where unrelated facts could see assigned homes.
That was useful while closing capability gaps, but now it makes the allocation
route hard to review and expensive to compile.

The desired structure should separate:

- value discovery and classification;
- register class and width decisions;
- interval ranking and physical assignment;
- stack-slot fallback;
- value-home publication;
- call/return ABI move resolution;
- phi/out-of-SSA move resolution;
- pointer carrier tracking;
- runtime-helper mapping;
- spill/reload publication.

## In Scope

- Extract focused files under `src/backend/prealloc/` or a
  `src/backend/prealloc/regalloc/` subdirectory, such as:
  - `regalloc/values.cpp`
  - `regalloc/classification.cpp`
  - `regalloc/allocator.cpp`
  - `regalloc/value_locations.cpp`
  - `regalloc/call_moves.cpp`
  - `regalloc/phi_moves.cpp`
  - `regalloc/pointer_carriers.cpp`
  - `regalloc/runtime_helper_mappings.cpp`
  - `regalloc/spill_reload.cpp`
- Keep the public regalloc phase entry stable.
- Preserve allocation output, move-resolution output, and prepared dumps.
- Use narrow private helper headers for shared internal data such as active
  assignments or program-point lookup.

## Out of Scope

- Changing allocation algorithms, register priorities, spill heuristics, ABI
  move semantics, or prepared output.
- Reworking stack layout.
- Moving prepared schema definitions; that belongs to the schema-header idea.
- Splitting prepared printer output.
- Adding new target capabilities while decomposing.

## Completion Criteria

- `regalloc.cpp` is no longer a 3000+ line multi-family implementation file.
- Allocation algorithm code is separated from call/return moves, runtime helper
  mapping, pointer carriers, and spill/reload publication.
- Existing focused and broad backend/prealloc tests pass without changed
  expected prepared output.
- The resulting ownership map makes it clear where a future prepared fact
  should be added.

## Closure Notes

The active runbook completed Steps 1 through 10. Focused helper owners now live
under `src/backend/prealloc/regalloc/` for classification, values, intervals,
assignment, stack slots, storage, value homes, pointer carriers, call/return
ABI, call moves, move records, phi moves, consumer moves, runtime helpers, and
spill/reload. `regalloc.cpp` remains the coordinator for phase ordering,
allocation ordering and mutation, and final prepared publication.

`src/backend/prealloc/README.md` documents the coordinator/helper ownership map
so future prepared facts have a clear owner.

Close-time backend regression guard passed with equal-green results:
`test_before.log` and `test_after.log` both reported 139/139 passing backend
tests, and the checker passed in non-decreasing mode for this refactor slice.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes allocation decisions or prepared dumps while claiming a pure split;
- hides algorithm changes inside file movement;
- duplicates helper logic instead of extracting a narrow shared helper;
- creates a new catch-all file that simply replaces `regalloc.cpp`;
- adds AArch64-specific shortcuts to shared regalloc;
- weakens tests or expected dumps to accept changed behavior.
