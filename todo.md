# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.2
Current Step Title: Reconcile allocation and strict downstream realization

## Just Finished

- Plan Step 9.2 is complete: the E4/allocated-owned non-mutating
  `FrameRealizationTransaction` now produces one exact-revision immutable
  frame/mapping plan before final target realizability. It covers every
  spill/reload, outgoing-call, static/dynamic frame, displacement, adjustment,
  implicit-action, and registered mapping-rule fact needed for one-record
  proof, or fails the enclosing transaction atomically before publication.
- F1 applies the verified frame plan and one registered mapping per node; it
  cannot choose placements, expand/materialize a record, or return repair to
  D4. Published D3/D4/initial-D5 `PseudoBir` now uses an allocation-free
  `PseudoPublicationGate`, while private E3 retry and D5-resolved candidates
  use `AssignedAllocationCandidateGate`, which admits/requires exact assignment
  and spill state without publishing another stage type.

## Suggested Next

- Execute Plan Step 11's legacy/core disposition repair, including the nested
  legacy-path inventory and the remaining C5-to-C6 address-owner correction.

## Watchouts

- Step 11 still must inventory current nested legacy paths and correct address
  ownership from C5 to C6.
- Preserve the Step 9.1 exact-current closure: the frame transaction runs after
  its projection/E1/E2/E3 products and before the final target-realizability
  key, which incorporates the exact `FrameRealizationKey`.
- After Step 11, repeat Step 12 and obtain a new independent Step 13 review.
  Step 14 is forbidden until that review reports zero blockers.

## Proof

- Passed the supervisor-selected documentation proof: `git diff --check`; the
  required positive `rg` over all twelve owned BIR/MIR architecture contracts;
  and the required negative `rg` rejecting F1 frame-offset/displacement choice,
  MIR frame/stack/spill repair or expansion, and the stale undifferentiated
  D1-D5 Pseudo allocation-fact rule.
- No build or test subset applies to this docs-only packet. Per the delegated
  boundary, regression logs were not created or modified.
