Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Assemble Closure Payload

# Current Packet

## Just Finished

Step 5 of `plan.md` replaced the Follow-Up Idea Payloads scaffold in
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` with concrete
route-sized follow-up payload rows.

The payloads now cover seven accepted routes:

- producer/source identity foundation
- select-chain and direct-global dependency identity
- memory/access semantic identity
- block-entry/current-block publication identity
- CFG edge publication and join-source identity
- call-boundary semantic source facts
- comparison/materialized-condition producer identity

Each row records a proposed `ideas/open/` filename prefix, route scope,
out-of-scope guardrails, acceptance criteria, proof route, and reviewer reject
signals. The proof-route recommendations are now route-specific and include
query-equivalence expectations against the existing prepared query surfaces.

## Suggested Next

Execute Step 6: assemble the close-ready payload in `todo.md`. Summarize the
artifact link, candidate table, reject table, dependency order, follow-up idea
payloads, and proof-route recommendations, then verify the run remained
analysis-only.

## Watchouts

- This plan remains analysis-only; do not edit implementation files or test
  expectations.
- Step 6 is closure packaging only; avoid reclassifying routes unless a concrete
  missing artifact requirement is found.
- The follow-up ideas are route-sized; consumer switches should wait for
  equivalent BIR-owned queries and use existing prepared queries as comparison
  oracles.
- Any future payload that imports a whole mixed `Prepared*Plan`,
  `Prepared*Publication`, `PreparedAddress`, or `PreparedValueHome` shape into
  BIR should be treated as route drift.

## Proof

Analysis-only/docs packet; no build required. Verification for this packet:
the Follow-Up Idea Payloads table has concrete proposed idea filename prefixes,
route scopes, out-of-scope guardrails, acceptance criteria, proof routes, and
reviewer reject signals for all seven accepted route groups; the Proof-Route
Recommendations section has route-specific query-equivalence and subset
guidance; `todo.md` records Step 5 completion and recommends Step 6. No
`test_after.log` was created because the delegated proof explicitly required no
build.
