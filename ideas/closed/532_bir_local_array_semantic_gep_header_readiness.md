# BIR Local-Array And Semantic-GEP Header Readiness

Status: Closed
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

## Closure Notes

Closed after the behavior-preserving aggregator-only
`src/backend/bir/bir_local_array_semantic_gep.hpp` split. `bir.hpp` remains the
compatibility aggregator and includes the focused header at the
prerequisite-safe boundary after `bir_memory_provenance.hpp`.

Direct consumer include replacement is intentionally parked outside this idea:
current consumers still need broader complete BIR core model, prealloc
publication, route, or lowering declarations. The accepted completion for this
idea is the safe declaration-surface extraction plus proof that no behavior,
record layout, storage, optionality, lookup, authority policy, or capability
tests changed.

Close proof used the focused backend subset for local-array proof,
semantic-GEP, scalar local-load, and static-GEP authority coverage:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|publication_plan_record|prepare_stack_layout)' > test_after.log 2>&1
```

Regression guard passed against the matching rolled-forward `test_before.log`
with `--allow-non-decreasing-passed`: 3/3 before and 3/3 after, no new
failures.
