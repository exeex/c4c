# RV64 Prepared Local-Memory Addressing For src/960209-1.c

Status: Open
Type: RV64 object-route capability repair
Parent: `ideas/closed/554_out_of_ssa_parallel_copy_move_bundle_publication.md`
Related: `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`
Owning Layer: RV64 prepared object local-memory addressing

## Goal

Make the RV64 object route consume prepared local-memory facts for the
`src/960209-1.c` first blocker by supporting prepared frame-slot or
pointer-value base-plus-offset local memory addressing where those facts are
already published.

## Why This Exists

The out-of-SSA parallel-copy publication route is complete. The one-row
`src/960209-1.c` scan no longer reports
`prepared_consumer_category=missing_move_bundle`; it now fails at a new first
blocker:

```text
[RV64_C4C_OBJ_COMPILE_FAIL]
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

This route owns the narrow RV64 object-route capability exposed by that
diagnostic. The broader bucket review in idea 547 remains open for classifying
other local-memory and call-metadata rows, but it is not an exact
implementation owner for this advanced `src/960209-1.c` blocker.

## In Scope

- Reproduce and inspect the current `src/960209-1.c` local-memory diagnostic.
- Determine whether the failing memory operation already has prepared
  frame-slot or pointer-value base-plus-offset facts available to the RV64
  object route.
- Implement RV64 object emission for the prepared local-memory addressing
  shape that is proven by current facts.
- Add or update focused backend tests for the supported prepared local-memory
  operand shape.
- Use the one-row RV64 gcc torture backend scan to prove the row moves past
  `unsupported_local_memory_access` or exposes a new auditable first blocker.

## Out Of Scope

- Guessing missing address provenance from raw target or testcase shape.
- Broad BIR or prepared producer reconstruction for unrelated local-memory
  rows.
- Call-metadata cleanup.
- F128 helper, ABI, or quarantine work unless fresh row-level evidence proves
  this route has become F128-primary.
- Expectation rewrites, unsupported marker edits, allowlist filtering, or
  runtime comparison changes.
- Filename-specific handling for `src/960209-1.c`.

## Acceptance Criteria

- The first failing local-memory operation is classified by available prepared
  facts: frame-slot base-plus-offset, pointer-value base-plus-offset, or a
  producer-owned missing-fact blocker.
- RV64 object emission handles the supported prepared local-memory addressing
  form without testcase-specific matching.
- Focused backend tests prove the object-route addressing behavior.
- The one-row `src/960209-1.c` scan no longer reports
  `unsupported_local_memory_access` for the same operation, unless a new
  diagnostic demonstrates a deeper producer-owned blocker.
- No gcc_torture expectations, unsupported markers, allowlists, or runtime
  comparison behavior are weakened.

## Reviewer Reject Signals

- Reject fixes that special-case `src/960209-1.c`, one block label, one local
  symbol, one stack slot, or one instruction shape instead of supporting a
  semantic prepared local-memory address form.
- Reject RV64 object lowering that reconstructs local memory addresses from
  raw target shapes when prepared facts are absent or contradictory.
- Reject BIR/prepared producer rewrites that are not required by the first
  local-memory operation exposed in the row log.
- Reject helper renames, diagnostic wording changes, or classification-only
  updates claimed as capability repair.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparisons as progress.
- Reject broad local-memory bucket work that should remain under idea 547
  unless this route first proves the same owner and scope.
