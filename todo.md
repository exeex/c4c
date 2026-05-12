# Current Packet

Status: Active
Source Idea Path: ideas/open/175_hir_typespec_ref_structured_equivalence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The HIR Type-Ref Family

## Just Finished

Lifecycle activation created the active runbook and initialized the canonical
execution scratchpad for Step 1.

## Suggested Next

Executor should inspect Step 1, choose one bounded HIR ordinary type-ref family,
and update this file with the selected implementation surface plus proof
command before making code changes.

## Watchouts

- Do not expand into LIR `LirTypeRef` equality; that belongs to idea 176.
- Do not expand into template record owner identity; that belongs to idea 177.
- Do not claim progress from helper renames, printed-text changes, or
  testcase-shaped matching while the old semantic comparison remains.
- Preserve syntax/display uses of `TypeSpec` unless the selected HIR path
  explicitly requires a tested behavior change.

## Proof

Lifecycle-only activation. No build or test run required.
