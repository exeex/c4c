# Entry Formal Publication Planning in Prealloc

## Intent

Move generic function-entry formal publication planning from AArch64 dispatch
into prealloc value-location and publication planning helpers.

## Why This Exists

AArch64 entry lowering currently plans how prepared formal homes are published
as machine values. The target must still own concrete ABI register and stack
copy emission, but the target-independent publication plan is a prepared value
location concern.

x86 function-entry lowering should be able to consume the same formal
publication plan instead of duplicating AArch64's prepared-home interpretation.

## Current AArch64-Owned Responsibility

Current responsibility is centered on `dispatch_entry_formals.cpp` and adjacent
publication/value-location helpers that turn prepared formal homes into machine
value publications before target instruction emission.

## Proposed Destination Layer

`src/backend/prealloc` should own a formal-publication planning helper that
maps prepared formal homes and value-location facts into target-neutral
publication records.

Targets should provide hooks or local code for concrete ABI source operands and
entry-copy instruction emission.

## x86/RISC-V Benefit

x86 can consume the same prepared formal-publication plan for function-entry
lowering while preserving x86 ABI source operands locally. RISC-V benefits
later from a target-neutral entry-publication contract when it adopts Prepared
function lowering.

## In Scope

- Extract target-neutral entry-formal publication records from current AArch64
  dispatch behavior.
- Keep target ABI source selection and concrete copy emission in AArch64.
- Convert AArch64 to consume the shared plan.
- Identify the matching x86 entry-lowering consumer path.
- Prove unchanged entry lowering across multiple formal locations.

## Out of Scope

- Changing the calling convention, parameter store strategy, or AArch64
  prologue behavior.
- Moving ABI register naming or stack argument addressing into prealloc.
- Combining this with general call-boundary move classification.
- Downgrading tests around formal publication failures.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs proof over more than one function-entry shape, including
different formal locations where supported, and must demonstrate unchanged
AArch64 output. The implementation must also show that missing or malformed
Prepared formal facts still produce equivalent or better diagnostics.

## Reviewer Reject Signals

Reject if target ABI register/stack policy migrates into prealloc, if only one
named formal pattern is handled, if tests are weakened, or if the extracted plan
cannot plausibly be consumed by x86 without copying AArch64 entry dispatch.

Reject if the old AArch64 publication logic is left active in parallel with a
new unused prealloc helper and the route claims capability progress.

## Completion Note

Closed after the active runbook moved target-neutral function-entry formal
publication planning into prealloc-owned helper records, adapted AArch64
entry-formal dispatch to consume the shared plan while retaining ABI source
selection and concrete copy emission locally, and exposed a concrete x86
prepared-query reuse path.

Validation covered the prealloc formal publication helper, the AArch64 consumer
path, and x86 prepared query reuse through backend tests including
`backend_prealloc_formal_publications`, `backend_aarch64_call_boundary_owner`,
and `backend_x86_prepared_decoded_home_storage`. No tests or expectations were
weakened or reclassified.
