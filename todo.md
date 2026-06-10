Status: Active
Source Idea Path: ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Draft Follow-Up Ideas And Proof Routes

# Current Packet

## Just Finished

Step 4 of `plan.md` completed bounded follow-up idea and proof-route synthesis
in `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.

The artifact now contains concrete follow-up boundaries for Routes 1-7, the
route-index facade, `PreparedFunctionLookups` aggregate privacy, and a separate
return-chain no-route follow-up. Each route records which cache/index group can
eventually become private, which prepared or target-policy surfaces must remain
public temporarily, required BIR/index prerequisites, consumer migration
requirements, proof-route recommendations, and reviewer reject signals.

## Suggested Next

Step 5 packet: perform the final Phase C consistency pass, still analysis-only.
Verify that the artifact cleanly separates private-cache candidates,
temporary public migration oracles, permanently public target-policy surfaces,
blocked BIR-route migrations, and the return-chain no-route gap. Then prepare
the plan-owner handoff recommendation for whether this runbook is complete,
blocked, or should become follow-up source ideas.

## Watchouts

- This plan is analysis-only; do not perform implementation changes during
  Phase C.
- `PreparedFunctionLookups` is a private-cache candidate only as an aggregate
  construction detail. It is not a replacement BIR route-index abstraction, and
  AArch64 still projects its fields directly.
- Return-chain lookup helpers are explicitly kept as a separate no-route
  follow-up; the consistency pass should preserve that boundary and not fold it
  into Route 1 producer identity or Route 7 comparison provenance.
- Route 3 remains split between BIR-ready semantic memory/access identity and
  target/layout address materialization policy.
- The route index facade is partial coverage for selected Route 4 and Route 7
  references, not a universal contraction surface for Routes 1, 2, 3, 5, or 6.
- Follow-up implementation ideas should migrate one consumer group at a time
  and keep prepared APIs as oracles until equivalence and negative cases are
  proven.

## Proof

Command: `printf 'analysis-only Step 4; no build/test required\n' > test_after.log`

Result: passed. This was an analysis-only packet; no build or test was required
by the delegated proof. Proof log path: `test_after.log`.
