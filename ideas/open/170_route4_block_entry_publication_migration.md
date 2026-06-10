# 170 Route 4 block-entry publication migration

## Goal

Move the residual block-entry publication consumer from the older semantic PHI
scan to Route 4 BIR publication availability records or a thin BIR-backed
facade.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 4 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Block-entry publication availability.
- Current-block/block-entry source producer and value-role identity.
- Route-index reference validation where already available.
- Prepared block-entry publication helpers only after the residual consumer no
  longer uses them.

## Out Of Scope

- Publication hook kind, destination home, storage encoding, stack-source
  extension, register-view conversion, immediate spelling, emitted storage,
  scalar publication emission policy, or publication ordering changes.

## Acceptance Criteria

- The residual block-entry MIR consumer reads `Route4PublicationAvailabilityIndex`
  or an equivalent BIR-backed facade.
- Current-block and block-entry oracle tests cover available and missing cases.
- Prepared block-entry helpers remain public until the migrated consumer no
  longer needs them as oracle or fallback.

## Proof Route

Use focused current-block and block-entry oracle tests plus route-index
reference validation, then run the MIR publication/query subset that exercises
the migrated block-entry consumer.

## Reviewer Reject Signals

- Moving publication emission mechanics into BIR.
- Hiding prepared block-entry helpers before the residual consumer switches.
- Proving only current-block success without block-entry negative coverage.
