# BIR Route Header Split After Body Moves

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 11 of 13, after `ideas/open/529_bir_route_facade_body_extraction.md`
Owning Layer: BIR route declaration surface
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Split route declarations into narrow headers only after the route body owners
are stable.

## Why This Exists

Idea 518 rejected an early one-shot `bir.hpp` breakup. Header movement should
come after body extraction proves route dependency direction and include
pressure.

## In Scope

- Move route declarations out of `bir.hpp` only where narrower headers reduce
  coupling.
- Update include sites required by compile proof.
- Preserve public names, signatures, namespaces, enum values, and behavior.
- Use `.codex/skills/c4c-clang-tools/` plus include/build evidence before
  selecting header boundaries.

## Out Of Scope

- Do not move implementation bodies in this header-focused slice.
- Do not create one large replacement monolith unless review proves it reduces
  include pressure.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not change route semantics or public query names.

## Acceptance Criteria

- Build proof passes for all backend targets that include `bir.hpp`.
- Focused backend route tests pass after include churn.
- Header split reduces or clarifies dependencies without forcing broad
  consumers to include many new route headers.

## Lifecycle Notes

### Parked after review checkpoint

Reviewer report `review/bir_route_header_split_review.md` found that the
current `bir_route_index.hpp` split is behavior-preserving but not a standalone
narrow dependency header. It is an aggregator-included declaration fragment
that depends on prerequisite BIR model and route declarations already supplied
by `bir.hpp`.

Idea 530 was parked rather than closed because direct include replacement had
been examined and was unsafe, and the dependency-reduction acceptance criteria
were not yet proven. Follow-up prerequisite work lived in
`ideas/open/533_bir_route_index_standalone_prerequisites.md`.

### Closed after remaining-candidate map

Idea 533 closed the route-index standalone prerequisite route by proving the
current safe boundary: `bir_route_index_prereqs.hpp` holds route4/route7 status
and role prerequisites, while `bir_route_index.hpp` remains an aggregator-only
fragment because `RouteIndexRecordReference` stores
`Route1SourceValueIdentity` by value.

The resumed remaining-candidate map for this idea found no additional route
declaration boundary that can reduce or clarify dependencies without pulling in
out-of-scope route1 identity/core-model ownership or later
memory-provenance/local-array work. Route 1, route 3, route 5, route 6, route
8, and the remaining route4/route7 records all depend on by-value route1
identity, core BIR model types, memory-provenance records, or broad function
surfaces. Direct include replacement remains unsupported.

Closure accepts the existing behavior-preserving header work as the useful
scope for idea 530. Any further dependency reduction should be owned by a
separate source idea for route1 identity/core-model prerequisite splitting or
record layout redesign, not by extending this header-split route.

Close proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1

python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, before=2 passed / 0 failed, after=2 passed / 0 failed.

## Reviewer Reject Signals

- Header split increases coupling or creates a new catch-all route monolith.
- Complete-type requirements for `Function` vectors break or rely on fragile
  forward declarations.
- Public API names, namespaces, or signatures change.
- Header work is mixed with semantic or body movement.
