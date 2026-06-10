Status: Active
Source Idea Path: ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify Phase B Route Contraction Status

# Current Packet

## Just Finished

Step 3 of `plan.md` completed Phase B route contraction classification in
`docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.

The artifact now contains a route-by-route status table for Routes 1-7, the
route-index facade, and the return-chain no-route gap. Each row classifies
cache contraction status, concrete blockers, prepared disposition, BIR
annotation prerequisites before privatization, and public prepared surfaces
that must remain temporarily.

## Suggested Next

Step 4 packet: synthesize the contraction policy from the Step 3 table into a
small implementation-order recommendation, still analysis-only. The packet
should identify which families are safe future private-cache candidates, which
must stay public migration oracles, which are permanently target-policy
surfaces, and which blocked routes need a prerequisite route migration or new
schema before any implementation slice is proposed.

## Watchouts

- This plan is analysis-only; do not perform implementation changes during
  Phase C.
- `PreparedFunctionLookups` is a private-cache candidate only as an aggregate
  construction detail. It is not a replacement BIR route-index abstraction, and
  AArch64 still projects its fields directly.
- Return-chain lookup helpers are explicitly classified as a no-route gap; Step
  4 should keep them separate from Route 1 producer identity and Route 7
  comparison provenance.
- Route 3 remains split between BIR-ready semantic memory/access identity and
  target/layout address materialization policy.
- The route index facade is partial coverage for selected Route 4 and Route 7
  references, not a universal contraction surface for Routes 1, 2, 3, 5, or 6.

## Proof

Command: `printf 'analysis-only Step 3; no build/test required\n' > test_after.log`

Result: passed. This was an analysis-only packet; no build or test was required
by the delegated proof. Proof log path: `test_after.log`.
