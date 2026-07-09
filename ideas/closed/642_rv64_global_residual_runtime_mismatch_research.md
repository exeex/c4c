# RV64 Global Residual Runtime Mismatch Research

Status: Closed
Type: Research
Parent: `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Related:
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
- `ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`
Owning Layer: RV64 runtime/object correctness evidence for global residual rows
Queue Order: 42
Prerequisites: object emission must already move the row past compile/link
before runtime mismatch ownership is assigned
Proof Surface: idea-631 residual rows that reach runtime mismatch instead of a
direct global-symbol local-memory diagnostic

## Goal

Determine the first concrete owner of the global-residual runtime mismatch row
after direct global-symbol local-memory policy has been ruled out.

## Why This Exists

Idea 631 Step 5 classified `src/pr79737-2.c` as
`RV64_BACKEND_RUNTIME_MISMATCH`. The row has already moved past the direct
local-memory object-emission blocker, so further work needs runtime/object
ownership evidence rather than another local-memory admission change.

## In Scope

- Reproduce `src/pr79737-2.c` with current object, link, and runtime logs.
- Capture enough evidence to distinguish bad global/local memory lowering,
  object relocation, stack layout, ABI/call setup, branch/control flow, or true
  runtime support.
- Compare the row against runtime mismatch ownership lanes from idea `618` and
  classify the first owner with concrete artifacts.
- If a single implementation owner is proven, create or recommend a separate
  implementation idea with a focused proof surface.

## Out Of Scope

- Reopening direct global-symbol local-memory support from idea `631`.
- Implementing a runtime fix in this research idea.
- Grouping all runtime mismatches into one generic runtime-support owner.
- Expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or
  pass/fail accounting changes.

## Acceptance Criteria

- The row is rerun from a fresh build with captured object/link/runtime
  artifacts.
- The first owner is classified as ABI, layout, local/global memory, object
  relocation, call lowering, branch/control flow, true runtime support, or
  unresolved with explicit missing evidence.
- Any recommended implementation follow-up is split by single owner and proof
  surface.
- The investigation does not claim progress from changing expectations,
  allowlists, timeout policy, or runtime comparison behavior.

## Reviewer Reject Signals

- Reject treating the runtime mismatch alone as proof of a runtime-support
  implementation fix.
- Reject implementation edits or broad runtime rewrites inside this research
  idea.
- Reject reopening idea `631` unless fresh evidence shows a missing direct
  global-symbol local-memory authority fact before object emission.
- Reject named-case-only fixes for `src/pr79737-2.c` without a semantic owner
  and focused proof route.
- Reject expectation, unsupported-marker, allowlist, timeout/accounting, or
  runtime-comparison changes as progress.

## Completion Notes

Closed after the research artifact
`docs/runtime_mismatch_ownership/04_global_residual_runtime_mismatch.md`
classified `src/pr79737-2.c` as packed bitfield/global-object layout and
bitfield access lowering, not true runtime support. The row was refreshed,
candidate owners were ruled in or out with object/link/runtime evidence, and
the published artifact identified a single implementation owner.

Follow-up implementation work has been split into
`ideas/open/651_rv64_packed_bitfield_global_layout_access.md` so this research
idea remains implementation-free.
