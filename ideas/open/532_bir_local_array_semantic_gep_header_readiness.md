# BIR Local-Array And Semantic-GEP Header Readiness

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 13 of 13, after `ideas/open/531_bir_memory_provenance_header_readiness.md`, before `ideas/open/519_rv64_object_emission_cleanup_umbrella.md`
Owning Layer: BIR local-array and semantic-GEP declaration surface
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Consider local-array and semantic-GEP analysis header splits after memory
provenance ownership is stable.

## Why This Exists

Local-array proof and semantic-GEP records may eventually move behind narrower
analysis headers, but doing so before memory ownership settles risks include
cycles and accidental semantic churn.

## In Scope

- Audit local-array proof records, semantic GEP records, scalar local-load
  consumers, and static GEP authority consumers.
- Move declarations only if include/build evidence supports a narrower public
  analysis header.
- Preserve record layout, vector storage, optionality, lookup behavior, and
  public names.
- Use `.codex/skills/c4c-clang-tools/` type-reference and caller/callee
  queries before selecting boundaries.

## Out Of Scope

- Do not move implementation behavior in the same slice.
- Do not change `Function` storage semantics.
- Do not change memory provenance authority policy.
- Do not fold this work into LIR-to-BIR producer capability.

## Acceptance Criteria

- Build proof passes.
- Focused local-array proof, semantic GEP, scalar local-load, and static GEP
  authority proof passes.
- Broad backend subset is considered if `Function` storage declarations are
  touched.

## Reviewer Reject Signals

- Header movement changes record layout, vector storage, optionality, or lookup
  behavior.
- New include cycles appear between core model, memory provenance, and route
  headers.
- Lowering behavior or capability tests are edited to justify the split.
- The slice proceeds before memory provenance ownership is settled.
