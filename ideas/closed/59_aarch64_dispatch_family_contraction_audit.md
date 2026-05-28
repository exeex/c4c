# AArch64 Dispatch Family Contraction Audit

## Goal

Audit the remaining AArch64 `dispatch*` codegen family after the prepared
authority repair sequence, then create follow-up cleanup ideas that reduce the
family without mixing mechanical layout contraction with semantic authority
migration.

## Why This Exists

Ideas 47 through 49 repaired the main duplicate prepared-authority paths in:

- `dispatch_edge_copies.cpp`
- `dispatch_publication.cpp`
- `dispatch_value_materialization.cpp`

Those closures indicate the owned semantic recovery paths now route through
prepared/shared authority, while AArch64 keeps target-local emission,
diagnostics, scratch, hazard, and instruction spelling. After idea 58 repairs
the current regression set, the next cleanup question is no longer "move all
dispatch logic to BIR" in one step. The question is which remaining dispatch
files are still justified owners and which should be folded back, split into
target-emission owners, or represented by new shared/BIR follow-up ideas.

This idea is a classification and follow-up splitting slice. It should not
directly perform the contraction.

## Dispatch Family To Audit

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`

## In Scope

- Review closure notes from ideas 47, 48, 49, and 58 before classifying the
  remaining dispatch family.
- Compare the current dispatch family against the reference ARM codegen layout
  and shared backend generation model.
- Classify each dispatch file or helper group as:
  - `fold-back`: no longer needs a standalone owner and should merge into a
    reference-style AArch64 owner.
  - `move-forward`: remaining semantic authority belongs in BIR/shared
    prepare/prealloc.
  - `keep-local`: target-local emission, scratch, clobber, diagnostics, or
    instruction spelling justifies the local owner.
  - `split-owner`: helper group should move to a more precise non-dispatch
    target owner.
  - `needs-more-evidence`: requires a narrow audit/proof before deciding.
- Produce numbered follow-up ideas for each coherent contraction or authority
  migration slice.
- Keep the follow-up ideas small enough that one can be implemented without
  revalidating the entire dispatch family at once.

## Out Of Scope

- Directly merging or deleting dispatch files.
- Reopening semantic repairs already closed by ideas 47 through 49 unless the
  audit finds a concrete residual duplicate-authority path.
- Repairing the current baseline regressions owned by idea 58.
- Broad AArch64 codegen layout cleanup outside the dispatch family.
- Treating line count reduction or file count reduction as success without a
  correct ownership classification.

## Acceptance Criteria

- A durable dispatch-family classification table exists.
- The table explicitly states which dispatch files are expected to disappear,
  merge, stay local, or require shared/BIR authority work.
- Follow-up `ideas/open/` files are created for the next contraction slices.
- Each follow-up idea names its owned files and whether it is mechanical
  fold-back, shared-authority migration, or target-owner split.
- The final close note explains how the plan avoids reintroducing raw producer
  scans or local fallback authority that 47 through 49 removed.

## Reviewer Reject Signals

- A patch changes implementation while claiming to only audit.
- A patch proposes merging every `dispatch*` file into `dispatch.cpp` without
  owner boundaries.
- A patch treats target-local scratch/clobber checks as semantic duplication
  without evidence.
- A patch ignores idea 58's regression boundary or uses this audit to repair
  those failures opportunistically.
- A patch creates vague follow-up ideas that do not name file families or the
  exact ownership class.

## Closure Note

Closed after the dispatch-family audit classified every file or helper group
named above and materialized the resulting follow-up route into
`ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md` through
`ideas/open/67_aarch64_local_slot_address_offset_probe.md`.

The durable classification table is recorded in `todo.md` history at commit
`8fad13a87`, and the final follow-up set is recorded at `f3bde145f`. The route
keeps the audit-only boundary intact: no implementation or test files changed
under this idea, and no compiler capability progress is claimed here.

The follow-up split avoids reintroducing raw producer scans or local fallback
authority removed by ideas 47 through 49 by keeping mechanical wrapper
fold-back, shared/BIR authority migration, target-owner relocation, local route
preservation, and evidence-only probing as separate source ideas. Shared
authority candidates such as same-block recursion, edge fallback, select-chain
dependency discovery, and join parallel-copy cache rebuilding were routed into
their own follow-ups instead of being hidden inside a bulk dispatch merge.
