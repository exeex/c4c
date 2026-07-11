# BIR Route Index Retirement Research Digest

This digest consumes the completed idea 693 research input package under
`docs/bir_route_index_retirement_research/` and supersedes the stale assumption
that `bir_route_index` is a general Route 1 through Route 8 public API or a
prepared authority source. The consumed research input files are:

- `index.md`
- `01_current_route_inventory.md`
- `02_required_bir_to_prealloc_inputs.md`
- `03_named_view_replacement_shape.md`
- `04_publication_and_authority_boundaries.md`
- `05_retirement_sequence.md`
- `06_test_and_dump_policy_after_route_retirement.md`
- `07_followup_idea_recommendations.md`
- `08_stack_view_and_destination_authority_handoff.md`

## Route Inventory Summary

The current route surface is not one uniform registry. `bir_route_index` is a
narrow compatibility facade over Route 4 publication validation and Route 7
comparison validation. Route 1, Route 2, Route 3, Route 5, Route 6, and Route 8
are separate route-numbered APIs with independent consumers.

The inventory classifies the route families this way:

- Route 1 is the same-block scalar producer and immediate-constant substrate
  used by other routes and target materialization paths.
- Route 2 is a select-chain and direct-global dependency view used by call and
  value materialization paths.
- Route 3 is memory access identity and same-block memory-source lookup used by
  publication and target memory agreement paths.
- Route 4 is current-block and block-entry publication availability, currently
  also carrying route-index proof attribution for prepared block-entry rows.
- Route 5 is CFG edge publication and current-block join-source reconstruction;
  its prepared contacts are executable prepared edge publications plus stored
  route agreement residue.
- Route 6 is call argument and call result source reconstruction for target
  call diagnostics and lowering checks.
- Route 7 is comparison instruction, operand, and branch-condition
  reconstruction, currently exposed through the route-index facade for AArch64
  comparison proof.
- Route 8 is return-chain value identity used by AArch64 return-chain helpers,
  not by `bir_route_index`.

The highest-value first retirement target is therefore facade/status residue:
`RouteIndexReferenceFacade`, `RouteIndexRoute`, `RouteIndexRecordReference`,
`Route4IndexReferenceValidation`, and `Route7IndexReferenceValidation`.

## Required BIR-To-Prealloc Inputs

The required codegen inputs are prepared-owned facts, not route-index status:
`PreparedFunctionLookups`, `PreparedValueHomeLookups`,
`PreparedEdgePublicationLookups`, `PreparedMoveBundleLookups`,
`PreparedValueFreshnessAuthority`, prepared call and address materialization
lookups, prepared memory access lookups, aggregate stack-source authority, and
`PreparedMirCoreView`.

Route 4 block-entry attribution, Route 5 join-source status/agreement, Route 7
validation status, and route-numbered dump names are proof/debug residue. When
diagnostics still need agreement with historical route facts, the agreement
should be recomputed locally at the diagnostic or target-proof surface instead
of stored as durable prepared authority.

This supersedes the stale API assumption that route-numbered status fields are
minimum BIR-to-prealloc inputs. The executable boundary is prepared data plus
BIR binding facts needed to relate prepared values back to source blocks and
instructions.

## Named View Replacement Shape

The replacement vocabulary should name ownership instead of route numbers:

- `BirProducerView` for producer identity and materialization availability.
- `BirMemoryAccessView` for memory access identity and memory-source lookup.
- `BirPublicationView` for current-block, block-entry, and CFG-edge BIR
  publication facts.
- `BirControlValueView` for select-chain and comparison/control facts.
- `BirCallBoundaryView` for call argument and result source facts.
- `BirReturnChainView` only if return-chain lowering remains target
  independent enough to keep a public view.
- `BirCompatibilityProofView` or route-specific proof adapters for temporary
  Route 4, Route 5, and Route 7 agreement status during migration.

Named BIR views may feed or validate prepared producers, but they must not add
route-numbered debug records to `PreparedFunctionLookups` or
`PreparedMirCoreView` as stable state. Current route builders can remain private
implementation details behind named wrappers while consumers migrate.

## Publication And Authority Boundaries

Publication and destination authority remain prepared-owned. Route 4, Route 5,
and Route 7 may describe BIR semantics or validate agreement, but they do not
authorize target storage, freshness, stack slots, move bundles, or MIR
destination selection.

Concrete executable authority belongs to prepared edge publications, prepared
move bundles, prepared value homes, selected freshness authority, aggregate
stack-source authority, branch stack-load authority, and prepared MIR views.
Route 4 block-entry attribution is a proof row for
`PreparedBlockEntryPublication`. Route 5 `route5_*` fields are trailing
agreement annotations for already-built prepared source facts. Route 7
validation is comparison/control proof, not publication or stack authority.

The reviewer rejection rule is explicit: any implementation that treats a
Route 4 status, Route 5 status/agreement, Route 7 validation result,
`RouteIndexReferenceFacade`, or dump row as executable authority is route drift.

## Retirement Sequence

The behavior-preserving sequence is staged:

1. Add named compatibility wrappers and proof adapters that forward to existing
   route builders without moving consumers.
2. Move the first low-risk consumer: Route 4 block-entry attribution in
   prepared lookup proof code.
3. Move Route 4 prepared-printer proof vocabulary without changing executable
   prepared behavior.
4. Move Route 7 AArch64 comparison facade consumers to a named comparison proof
   adapter.
5. Move Route 5 current-block join-source agreement behind named publication
   proof while keeping executable prepared edge publication unchanged.
6. Privatize or delete the narrow `bir_route_index` facade only after Route 4
   and Route 7 consumers no longer require public route-index status.
7. Retire route-numbered dump spelling under a dedicated dump-policy packet.

Route 1, Route 2, Route 3, Route 6, and Route 8 should remain deferred until
their named producer, control, memory, call, and return view replacements have
owned migration plans. Route 5 should not move early because it combines real
BIR publication semantics with stored proof residue.

## Test And Dump Policy

Post-retirement correctness should be proven above route dumps whenever the
change affects executable behavior. Preferred proof order is runtime or
object-runtime, object emission, target MIR or prepared-MIR, prepared contract
or prepared dump, and only then named BIR route-view proof for a BIR semantic
view or temporary compatibility adapter.

`--dump-bir` should remain semantic BIR and route-trace-free. `--dump-prepared-bir`
should expose prepared authority and named diagnostic proof. Transitional
labels such as `route4_*`, `route5_status`, `route5_agrees`, and Route 7
route-index status should be rewritten to named block-entry publication,
current-block join-source, edge-publication, or comparison agreement vocabulary
once the named proof surfaces exist.

New route-view tests are justified only for named BIR view contracts with a
planned deletion or rewrite point. Expectation downgrades, unsupported-marker
changes, allowlist changes, or baseline-only acceptance are not retirement
progress.

## Follow-Up Recommendations

The recommended dependency order is:

1. Introduce named BIR compatibility views and proof adapters.
2. Move Route 4 prepared lookup attribution to named publication proof.
3. Move Route 7 AArch64 comparison validation to named comparison proof.
4. Move Route 4 prepared-printer rows to named block-entry publication
   agreement.
5. Move Route 5 current-block join-source agreement behind named publication
   proof.
6. Privatize or delete the narrow `bir_route_index` facade.
7. Rewrite transitional dump and test vocabulary.
8. Extract broader named BIR views route family by route family.
9. Revisit stack destination authority only after explicit prepared authority
   prerequisites exist.

Ideas 647 and 655 should remain blocked or be rewritten unless they can show
positive prepared producer evidence above route dumps.

## Stack View And Destination Authority Handoff

The stack handoff is prepared-owned. BIR semantic views describe source-program
facts; prealloc/prepared producers turn those facts into frame layout, value
home, move bundle, freshness, aggregate stack, branch stack-load, and explicit
destination authority records; MIR consumes only those prepared records.

The first stack follow-up should define prepared stack views over existing
contacts: `PreparedStackLayout`, `PreparedFramePlanFunction`,
`PreparedValueHome`, `PreparedMoveBundle`, `PreparedMoveResolution`,
`PreparedValueFreshnessAuthority`, `PreparedAggregateStackSourceAuthority`,
`PreparedBranchStackLoadAuthority`, and prepared MIR direct-edge source views.

MIR must fail closed on missing destination homes, unsupported homes, ambiguous
fan-in, missing or invalid selected freshness, incomplete aggregate stack
authority, or route-only evidence. Route 4, Route 5, Route 7, facade status, and
dump rows may feed a prepared producer only through named BIR views; they are
not direct MIR-side destination authority.

## Superseded Assumptions

This digest supersedes these stale route-numbered API assumptions:

- `bir_route_index` is not a registry for all numbered BIR routes; it is a
  Route 4/Route 7 compatibility facade.
- Route-index validation status is not prepared publication, freshness,
  move-bundle, stack destination, or MIR authority.
- Route 4 block-entry attribution and Route 5 `route5_*` fields are not durable
  prepared state; they are compatibility proof until named diagnostics replace
  them.
- Route 7 comparison validation must stay in comparison/control proof and must
  not authorize publication, freshness, move execution, or stack destinations.
- Route dumps are not sufficient acceptance proof for executable behavior when
  prepared, MIR, object, object-runtime, or runtime surfaces can prove the same
  contract.
