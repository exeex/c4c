# BIR Route Index Standalone Prerequisites

Status: Closed
Type: Behavior-preserving prerequisite cleanup
Parent: `ideas/open/530_bir_route_header_split_after_body_moves.md`
Owning Layer: BIR route declaration surface
Source Artifact: `review/bir_route_header_split_review.md`

## Goal

Make the route-index declaration boundary ready for standalone include use, or
prove that it should remain an internal `bir.hpp` fragment.

## Why This Exists

Idea 530 created `src/backend/bir/bir_route_index.hpp`, but review found it is
included from inside `bir.hpp` after prerequisite declarations are already in
scope. Direct include replacement is unsafe because the fragment depends on
core BIR model types, route1 identity declarations, route4 publication records
and statuses, route7 comparison records and statuses, `BlockLabelId`, and
standard library declarations supplied upstream.

Continuing direct include replacement under idea 530 would drift. This idea
owns the prerequisite mapping and any minimal declaration splits needed before
`bir_route_index.hpp` can be considered a standalone narrow header.

## In Scope

- Map every declaration and complete-type dependency that prevents
  `bir_route_index.hpp` from compiling as a top-level include.
- Decide whether prerequisite route/model declarations can be split into
  focused headers without broadening consumer include burden.
- Add namespace wrappers, minimal standard/library includes, and focused
  prerequisite includes only when mapping proves they are needed and stable.
- Keep `bir.hpp` as the compatibility aggregator during the transition.
- Update include sites only after a direct compile probe proves the route-index
  header is standalone and the consumer does not require the broader BIR
  surface.

## Out Of Scope

- Do not move implementation bodies.
- Do not move core BIR model ownership for `Value`, `Inst`, `Block`,
  `Function`, `Module`, or `MemoryAddress`.
- Do not force direct include replacement at MIR, prealloc, or BIR tests that
  still need broad model or route surfaces.
- Do not change public names, namespaces as observed through `bir.hpp`,
  signatures, enum values, or behavior.
- Do not merge memory-provenance or local-array semantic-GEP header readiness
  work into this prerequisite.

## Acceptance Criteria

- `bir_route_index.hpp` either compiles as a direct top-level include with
  stable prerequisite includes, or the idea records a proof-backed decision to
  keep it aggregator-only.
- Any prerequisite declaration splits are behavior-preserving and backed by
  clang symbol/type-reference evidence.
- Include-site replacements happen only where they reduce or clarify
  dependencies without requiring several route headers to recreate the old
  `bir.hpp` surface.
- Build proof passes for backend targets that include the changed BIR headers.
- Focused BIR route tests chosen by the supervisor pass after any header or
  include churn.

## Reviewer Reject Signals

- A slice claims dependency reduction while `bir_route_index.hpp` still cannot
  compile as a top-level include.
- Direct include replacements add multiple BIR route/model headers at a
  consumer merely to reconstruct the old broad `bir.hpp` surface.
- Prerequisite splitting relies on fragile forward declarations for complete
  types, vectors of complete types, or dereferenced `Block`/`Function` data.
- Public API names, signatures, enum values, or behavior change.
- Implementation bodies move as part of this header-prerequisite work.
- Tests or expectations are weakened, downgraded to unsupported, or rewritten
  without corresponding capability-preserving header evidence.
- The old route-index dependency problem is retained behind a renamed
  catch-all header or another aggregator-only abstraction while being claimed
  as standalone progress.

## Completion Notes

Closed after Step 4 lifecycle checkpoint.

The route-index standalone question is answered for this prerequisite idea:
`bir_route_index.hpp` remains aggregator-only for now, backed by the direct
top-level include probe recorded in `todo.md`. The remaining blocker is
`RouteIndexRecordReference::value`, which stores `Route1SourceValueIdentity`
by value and therefore requires that complete type rather than only a forward
declaration.

Implemented prerequisite movement split route4/route7 status and role
declarations plus supporting includes into
`src/backend/bir/bir_route_index_prereqs.hpp`, keeping `bir.hpp` as the
compatibility aggregator. No implementation bodies, route semantics, public
API names, signatures, or enum values changed.

Close gate:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, before=2 passed / 0 failed, after=2 passed / 0 failed.

Follow-up route1 identity prerequisite work is intentionally not folded into
this source idea. It should be opened as a separate idea only when the
supervisor chooses to pursue the remaining complete-type blocker by changing
the route1 identity declaration boundary or the route-index record layout.
