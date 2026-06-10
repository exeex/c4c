Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Assemble Closure Payload

# Current Packet

## Just Finished

Step 6 of `plan.md` assembled the close-ready payload for source idea
`ideas/open/151_phase_a_bir_normalization_candidate_audit.md`.

Close-ready artifact:
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

The artifact is complete for the Phase A analysis-only contract:

- `## BIR-Normalization Candidates` contains the accepted candidate table.
- `## Facts Rejected From BIR Normalization` contains the reject table and
  non-BIR owner boundaries.
- `## Dependency Order` records the ordered route groups, dependencies, switch
  timing rule, and schema rule.
- `## Follow-Up Idea Payloads` contains seven route-sized follow-up idea
  payload rows.
- `## Proof-Route Recommendations` records route-specific proof guidance with
  query-equivalence expectations.

The seven accepted follow-up routes are:

- producer/source identity foundation
- select-chain and direct-global dependency identity
- memory/access semantic identity
- block-entry/current-block publication identity
- CFG edge publication and join-source identity
- call-boundary semantic source facts
- comparison/materialized-condition producer identity

No implementation files, backend expectations, or source idea files were
changed by this closure packet.

## Suggested Next

Supervisor should treat this runbook as exhausted and ask the plan owner to
decide whether to close the active lifecycle state or file/split the follow-up
ideas from the completed artifact.

## Watchouts

- This plan remains analysis-only; do not edit implementation files or test
  expectations.
- The follow-up ideas are route-sized; consumer switches should wait for
  equivalent BIR-owned queries and use existing prepared queries as comparison
  oracles.
- Any future payload that imports a whole mixed `Prepared*Plan`,
  `Prepared*Publication`, `PreparedAddress`, or `PreparedValueHome` shape into
  BIR should be treated as route drift.

## Proof

Analysis-only/docs packet; no build required and no `test_after.log` created.

Verification for this packet:

- confirmed the artifact has linkable sections for candidate table, reject
  table, dependency order, follow-up idea payloads, and proof-route
  recommendations
- confirmed the follow-up payloads cover all seven accepted routes
- confirmed the proof-route recommendations are route-specific and include
  query-equivalence guidance
- confirmed `git diff --name-only` touches only owned files
- confirmed no implementation files or backend test expectations changed
