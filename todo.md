# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Plan Step 9.2 reconciled E1-E4 and MIR with the chosen D5
  allocation-aware copy-resolution route.
- E1 now models simultaneous-copy liveness plus every `CopyScratch`
  reservation and exclusion; E2 assigns each reservation a finite legal
  non-spillable, non-aliasing home or fails closed.
- E3 preserves `ParallelCopy` and scratch-reservation semantics through every
  retry, cannot spill scratch or resolve bundles, and yields the sole stable
  candidate accepted by D5's subordinate resolver.
- E4 accepts only the exact resolved revision and
  `CopyResolutionFingerprint`; `MirReadyBirView` contains no unresolved copy
  intermediate, and MIR maps every remaining non-`InlineAsm` node, including
  `EdgeCopy`, one-to-one without allocation, scratch, expansion, or repair.

## Suggested Next

- Execute Plan Step 10, "Freeze per-revision constraint projection ownership."

## Watchouts

- The D5 copy-resolution output creates a new exact revision. Step 10 still
  owns the single general mechanism by which the constraint product is
  preserved or reprojected for D1, D2, D4, initial D5, every E3 retry, and the
  resolved revision; this packet only requires exact current products at its
  adjacent boundaries.
- Stable IDs never establish product freshness. E4 and MIR must continue to
  reject any predecessor-revision constraint, liveness, assignment, spill, or
  copy-resolution fingerprint.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && rg -n 'CopyScratch|ParallelCopy|EdgeCopy|CopyResolution|E1|E2|E3|E4|liveness|interference|non-spillable|alias|retry|stable|directly realizable|MIR|repair|transaction|fingerprint' src/backend/bir/analysis/liveness/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/spill_reload/README.md src/backend/bir/allocated/README.md src/backend/bir/verify/README.md src/backend/bir/README.md src/backend/mir/README.md && ! rg -n 'MIR.*(resolve|schedule|repair).*(ParallelCopy|copy bundle)|ParallelCopy.*(maps|lowers).*(one|single).*machine instruction|CopyScratch.*spill' src/backend/bir/analysis/liveness/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/spill_reload/README.md src/backend/bir/allocated/README.md src/backend/bir/verify/README.md src/backend/bir/README.md src/backend/mir/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
