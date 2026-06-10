Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Draft Follow-Up Idea Payloads

# Current Packet

## Just Finished

Step 4 of `plan.md` replaced the dependency-order scaffold in
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` with concrete
route groups for Phase B and future implementation ideas.

Ordered route groups now cover:

- producer/source identity foundation
- select-chain and direct-global dependency identity
- memory/access semantic identity
- block-entry and current-block publication identity
- CFG edge publication and join-source identity
- call-boundary semantic source facts
- comparison and materialized-condition producer identity

Each route records prerequisites, BIR schema/annotation decisions, earliest
consumer switch timing, and Step 3 reject boundaries. The ordering keeps
producer/source identity before publication/edge, call, memory, and comparison
consumers, while placing memory identity before edge/call rows that may cite
memory-access source facts.

## Suggested Next

Execute Step 5: draft follow-up idea payloads in the artifact. Convert the
ordered Step 4 route groups into concrete `ideas/open/` payload rows with
scope, out-of-scope boundaries, acceptance criteria, proof routes, and reviewer
reject signals.

## Watchouts

- This plan remains analysis-only; do not edit implementation files or test
  expectations.
- Step 5 should preserve Step 4 ordering and Step 3 reject boundaries when
  drafting follow-up payloads.
- Follow-up ideas should be route-sized; consumer switches should wait for
  equivalent BIR-owned queries and use existing prepared queries as comparison
  oracles.
- Any payload that imports a whole mixed `Prepared*Plan`,
  `Prepared*Publication`, `PreparedAddress`, or `PreparedValueHome` shape into
  BIR should be treated as route drift.

## Proof

Analysis-only/docs packet; no build required. Verification for this packet:
the Dependency Order section was replaced with ordered route groups,
dependencies, switch timing, and schema/annotation decisions; the route
boundaries preserve Step 3 rejects for homes, stack offsets, register spelling,
ABI placement, target addressing, TLS relocations, parallel-copy schedule,
scratch policy, storage hooks, and final instruction records; `todo.md`
records Step 4 completion and recommends Step 5. No `test_after.log` was
created because the delegated proof explicitly required no build.
