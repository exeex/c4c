# BIR To Prealloc Rebuild Implementation

Status: Open
Type: Phoenix rebuild stage
After: `ideas/open/681_bir_to_prealloc_rebuild_replacement_draft.md`
Parent: `ideas/open/681_bir_to_prealloc_rebuild_replacement_draft.md`
Stage: 4 of 4
Read This First: `.codex/skills/phoenix-rebuild/SKILL.md`
Handoff Directory: `docs/bir_to_prealloc_rebuild/implementation/`
Related:
- `docs/bir_to_prealloc_rebuild/replacement_draft/`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`

## Intent

Convert the reviewed replacement drafts into real implementation through
staged migration. The goal is to replace the tangled `BIR -> prepared/prealloc`
handoff with explicit ownership seams and consumer views while preserving
behavior outside the active migration slice.

## Stage In Sequence

Stage 4 of 4. This is the only phoenix stage that owns real implementation
changes.

## Produces

- Real `.cpp` / `.hpp` files matching the reviewed stage-3 draft set.
- Dispatcher and ownership rewiring that routes migrated behavior through the
  new seams.
- Per-slice proof notes under
  `docs/bir_to_prealloc_rebuild/implementation/`.
- Legacy deletion or retirement only after the new owner is live and proved.

## Does Not Yet Own

- Reopening the stage-2 layout without an explicit repair.
- Unreviewed replacement files not named by stage 3.
- Test expectation downgrades, unsupported-marker changes, allowlist filtering,
  timeout changes, or runtime behavior weakening.
- Filename-specific or testcase-shaped repairs.

## Unlocks

Unlocks later focused capability ideas once the new BIR->prealloc seams are in
use. Ideas 647 and 655 may be reconsidered only if the new implementation
produces concrete authority/provenance facts that expose a positive producer
mechanism.

## Scope Notes

Migration should proceed in small proof-backed slices:

1. shared input/view types;
2. analysis phase orchestration;
3. one authority/provenance publication family;
4. one storage/home/value-location family;
5. one call/ABI/helper family;
6. one target/MIR consumer view;
7. dispatcher rewiring;
8. deletion of now-dead legacy paths.

Each packet must state what responsibility moved, what remains legacy, and
what proof covers the moved seam.

## Boundaries

- Keep the legacy path available until the replacement seam is live and proved.
- Do not delete a legacy path based only on "we think nothing uses this".
- Do not claim progress from a single previously failing testcase.
- During active migration, the supervisor may set
  `test_baseline_exclude_regex` in `.plan_review_state.json` to exclude only
  tests whose first owner is inside the currently migrated BIR/prealloc seam.
  The regex must be recorded in `todo.md` or the implementation proof notes,
  and it must be cleared or narrowed when the slice completes.

## Completion Signal

This idea can close only when:

- the reviewed draft set has been converted into real source;
- the new ownership seams are used by the active backend path;
- remaining legacy code is explicitly classified as live compatibility,
  deferred migration, or dead code removed through a proof-backed deletion
  packet;
- proof shows migrated capability families still work;
- any temporary `test_baseline_exclude_regex` is cleared or reduced to only
  explicitly deferred migration scope;
- no expectation downgrade, unsupported-marker edit, allowlist filtering,
  timeout change, runtime weakening, or testcase-shaped shortcut is claimed as
  rebuild progress.

## Reviewer Reject Signals

- Reject migration that moves code without moving ownership.
- Reject new god objects or giant contexts under new names.
- Reject MIR consumers that still query arbitrary prepared internals.
- Reject deletion of legacy paths without proof that the replacement path owns
  the seam.
- Reject broad or stale baseline exclusions.
- Reject named-case shortcuts, expectation rewrites, unsupported downgrades,
  allowlist filtering, or weaker runtime checks as progress.
