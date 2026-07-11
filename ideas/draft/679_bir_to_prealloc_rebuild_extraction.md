# BIR To Prealloc Rebuild Extraction

Status: Open
Type: Phoenix rebuild stage
After: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Stage: 1 of 4
Read This First: `.codex/skills/phoenix-rebuild/SKILL.md`
Handoff Directory: `docs/bir_to_prealloc_rebuild/extraction/`
Related:
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_route*.cpp`
- `src/backend/prealloc/`
- `scripts/plan_review_state.py`

## Intent

Extract the current `BIR -> prepared/prealloc` subsystem into compressed
markdown evidence before any rewrite. Treat the legacy implementation as a
behavior reference and responsibility inventory, not as the replacement design.

## Stage In Sequence

Stage 1 of the phoenix rebuild series:

1. extraction
2. extraction review and replacement layout
3. replacement draft and draft review
4. draft-to-implementation conversion

This stage must run after the `LIR -> BIR` adapter boundary umbrella is
available so the rebuild can depend on a named upstream boundary.

## Produces

- One `.md` companion for every selected legacy `.cpp` in the BIR route and
  prealloc/prepared scope.
- One `.md` companion for the single selected non-helper directory-index
  header in each extracted directory.
- One directory-level index:
  `docs/bir_to_prealloc_rebuild/extraction/index.md`.
- A header inventory in the directory-level index for headers that are
  important evidence but are not selected as phoenix index headers.

Default extraction set:

- `src/backend/bir/bir_route*.cpp`
- `src/backend/bir/bir_route_facade.cpp`
- `src/backend/prealloc/*.cpp`
- `src/backend/prealloc/regalloc/*.cpp`
- `src/backend/prealloc/prepared_printer/*.cpp`

Default formal index headers:

- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/prepared_printer.hpp`

Do not glob every `.hpp` in `src/backend/prealloc/`; the phoenix header policy
allows only one non-helper index header per directory.

## Does Not Yet Own

- Replacement architecture decisions.
- New `.cpp.md` / `.hpp.md` replacement drafts.
- Implementation edits, behavior changes, or test expectation changes.
- Deleting legacy `.cpp` files.
- MIR consumer rewiring.

## Unlocks

Unlocks stage 2 by giving reviewers a compressed, complete map of current
entry points, hidden dependencies, publication phases, special cases, and
consumer surfaces.

## Scope Notes

Extraction must classify special cases as one of:

- core logic
- optional fast path
- legacy compatibility
- overfit to reject

Capture these responsibility buckets at minimum:

- BIR route facts and route-index cross references.
- Prepared module aggregate state.
- Legalization and type/ABI repair.
- Control-flow, liveness, out-of-SSA, and regalloc state.
- Frame, storage, addressing, value-home, and object-data plans.
- Call, variadic, helper, intrinsic, atomic, inline-asm, and publication plans.
- Prepared printer and debug/dump-only surfaces.
- MIR/RV64/AArch64/x86 consumer dependencies on prepared facts.
- Stack destination fan-in authority pressure from ideas 647 and 655.

## Boundaries

- Do not edit implementation or tests in this stage.
- Do not activate or close ideas 647, 655, or 678 from this stage.
- Do not treat transient `build/` artifacts as canonical lifecycle state.
- If a baseline hook is needed while this rebuild is active, the supervisor may
  set `test_baseline_exclude_regex` in `.plan_review_state.json` through
  `scripts/plan_review_state.py set-baseline-exclude-regex`. The exclusion must
  be limited to in-scope BIR/prealloc rewrite tests and cleared when the stage
  is no longer active.

## Completion Signal

This idea can close only when:

- every selected legacy `.cpp` has a corresponding compressed `.cpp.md`;
- every selected directory-index `.hpp` has a corresponding `.hpp.md`;
- `docs/bir_to_prealloc_rebuild/extraction/index.md` links the full artifact
  set and records the non-selected header inventory;
- the extraction identifies APIs, contracts, hidden dependencies, consumer
  surfaces, and special-case classifications;
- no implementation, test expectation, unsupported marker, allowlist, runtime
  behavior, tracked build artifact, `plan.md`, or `todo.md` change is claimed
  as extraction progress.

## Reviewer Reject Signals

- Reject source dumps that are not compressed responsibility evidence.
- Reject extraction that ignores BIR route facts, prepared aggregate state, or
  MIR consumer dependencies.
- Reject more than one non-helper index header per extracted directory.
- Reject direct implementation, expectation rewrites, unsupported-marker
  changes, allowlist filtering, or runtime behavior changes in this stage.
- Reject baseline exclusions that outlive the active rebuild stage or hide
  unrelated failures outside the selected BIR/prealloc scope.
