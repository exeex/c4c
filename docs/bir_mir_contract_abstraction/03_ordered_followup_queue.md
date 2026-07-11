# Ordered Follow-Up Queue

This Step 3 queue converts the inventory and named contracts into single-owner
implementation ideas.  It is ordered by contract production and consumption,
not by route number.  A later idea may begin only when its listed contract
dependency is real and proved; route-numbered adapters may remain private
during migration but may not become new public authority.

## Dependency Order

| Order | Idea | First owning layer | First migrated consumer | Proof surface | Depends on |
|---:|---|---|---|---|---|
| 1 | [704 BIR semantic handoff views](../../ideas/open/704_bir_semantic_handoff_views.md) | BIR semantic-view producer | common MIR producer query | BIR contract/unit tests and public-header guard | umbrella contracts |
| 2 | [705 prepared fact boundary](../../ideas/open/705_prepared_fact_boundary_from_bir_views.md) | prealloc/prepared producer | `publication_plans.cpp` | prepared publication/call/lookup tests | 704 |
| 3 | [706 common MIR query migration](../../ideas/open/706_common_mir_named_query_migration.md) | common MIR query layer | `mir/query.cpp` | common query plus target handoff compile/tests | 704, 705 |
| 4 | [707 positive stack-authority producer gate](../../ideas/open/707_prepared_stack_destination_authority_positive_gate.md) | prealloc/prepared producer | fail-closed prepared MIR feature view | prepared contract and positive/negative object/runtime proof | 705, 706 |
| 5 | [708 x86 materializer cleanup](../../ideas/open/708_x86_named_handoff_materializer_cleanup.md) | x86 MIR materializer | `x86/module/module.cpp` | x86 handoff and route-debug behavior tests | 706 |
| 6 | [709 AArch64 materializer cleanup](../../ideas/open/709_aarch64_named_handoff_materializer_cleanup.md) | AArch64 MIR materializer | `aarch64/codegen/dispatch.cpp` | AArch64 dispatch/call/branch/join/scalar/memory tests | 706 |
| 7 | [710 RV64 materializer cleanup](../../ideas/open/710_rv64_named_handoff_materializer_cleanup.md) | RV64 MIR materializer | `prepared_edge_publication_emit.cpp` | prepared edge-publication and object-emission tests | 705, 706 |
| 8 | [711 route implementation quarantine](../../ideas/open/711_bir_route_implementation_quarantine.md) | BIR internals | private named-view adapters/builders | include/dependency guard plus BIR contract tests | 705, 706, 708-710 |
| 9 | [712 trailing debug/test vocabulary cleanup](../../ideas/open/712_route_debug_and_test_vocabulary_cleanup.md) | backend proof/test surface | direct route fixtures and route-labelled dumps | backend test suite and zero-public-vocabulary guard | 708-711 |

Ideas 708-710 are siblings after 706, not a route-number sequence.  They may
execute independently because each target owns distinct fallback logic and
proof surfaces.  Idea 707 is a producer gate, not permission to resume ideas
647 or 655: those parked ideas remain blocked until 707's complete positive
row and fail-closed proof are accepted.  Their later lifecycle review must
also decide whether their source intent is still needed.

## Ownership And Retirement Rules

Every generated idea records one first owner, its first real consumer, a proof
surface, dependencies, acceptance criteria, reject signals, and this shared
retirement guard:

```sh
rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index" \
  src/backend/bir src/backend/prealloc src/backend/mir tests
```

The guard is expected to shrink in ownership order.  Idea 704 removes route
types from the new public BIR seam; 705 removes them from executable prepared
records; 706 removes common MIR direct queries; 708-710 remove target
dependencies; 711 confines or retires the implementation vocabulary; and 712
removes the final debug, fixture, and label residue.  A green narrow test does
not satisfy an idea when the guarded consumer still imports a route header or
branches on a route record.

## Cross-Idea Acceptance Rules

- Named views must expose narrow semantic facts, never a complete `RouteN*`
  record, builder index, or unrestricted prepared module under a new name.
- Prepared facts own frame, home, move, freshness, publication, stack-source,
  branch-load, call-plan, and destination decisions.  BIR views are input
  evidence only.
- Common MIR and targets fail closed on missing, invalid, unsupported, or
  ambiguous prepared authority; no route fallback may survive migration.
- Target work changes materialization only after the common query contract is
  available.  It must not recreate route analysis locally.
- Debug and tests follow consumer migration.  Renamed expectations, weaker
  unsupported contracts, allowlists, or testcase-shaped shortcuts are reject
  signals, not proof.
- Route quarantine occurs after public consumers are gone.  Private builders
  may remain only when named-view tests and dependency guards prove that they
  cannot cross into prealloc, MIR, targets, or public fixtures.

## Coverage Of The Required Families

The queue covers the BIR semantic handoff (704), prepared fact boundary (705),
common MIR query migration (706), positive stack-authority producer gate
(707), target materializer cleanup split by x86/AArch64/RV64 ownership
(708-710), route quarantine (711), and trailing debug/test vocabulary cleanup
(712).  No generated idea combines producer definition, prepared production,
common consumption, target materialization, and proof cleanup.
