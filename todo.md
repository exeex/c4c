Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Candidate BIR Relationships

# Current Packet

## Just Finished

Step 2 of `plan.md` classified the BIR-normalization candidate table in
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

Completed candidate domains:

- same-block producer/materialization
- select-chain dependency/materialization
- current-block entry publication consumption
- edge-publication and parallel-copy source identity
- call-boundary source/dependency identity
- memory/access source identity
- comparison/materialized-condition producer identity

Each row now records current owner, current consumers, proposed BIR owner
surface, semantic rationale, and dependencies/risks where the current prepared
surface mixes BIR-normalizable identity with prealloc or target policy.

## Suggested Next

Execute Step 3: classify rejected prepared facts in the artifact. Focus on ABI
placement, stack offsets/frame slots, register spelling/banks/views, scratch
and aggregate transport policy, target addressing modes/TLS relocations, and
final instruction routing/order.

## Watchouts

- This plan is analysis-only; do not edit implementation files or test
  expectations.
- Treat target-specific placement, register spelling, scratch policy, final
  emission, and target addressing facts as reject candidates unless a separate
  target-neutral semantic relationship is proven.
- Step 2 classified only target-neutral relationship identity as BIR
  candidates. The same rows intentionally call out mixed payloads that Step 3
  should reject rather than carry into BIR.
- Call, edge-publication, and memory/addressing surfaces are the riskiest mixed
  domains because they colocate semantic source/access identity with ABI,
  home, stack, register, TLS, addressing-mode, and copy-routing policy.

## Proof

Analysis-only/docs packet; no build required. Verification for this packet:
artifact table filled for all seven required Step 2 domains, `todo.md` records
Step 2 completion, and final diff touches only
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` and `todo.md`.
