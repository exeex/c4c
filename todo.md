Status: Active
Source Idea Path: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Scalar Helper Ownership

# Current Packet

## Just Finished

Lifecycle activation only. No executor packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: map scalar helper ownership, record the move set
and parked set, and write the exact Step 2 validation command into this file.

## Watchouts

- Keep select-edge publication, predecessor publication movement, broad
  dispatch, and `fragment_for_prepared_instruction` out of this cleanup.
- Preserve compare normalization, branch-label fixups, diagnostics, emitted
  bytes, tests, expectations, and unsupported markers.
- Do not touch `review/global_address_helper_cleanup_review.md`.

## Proof

No validation run. Activation is lifecycle-only.
