# Ownership And Named Handoff Contracts

This Step 2 contract turns the current dependency inventory into ownership-
named boundaries.  The names below are proposed contracts, not implementation
added by this umbrella.  A contract is narrow only when it exposes the fact a
consumer needs, its stable identity, and an explicit availability result.  A
wrapper around a complete `RouteN*` record is still a compatibility bridge.

## Ownership Classification

| Class | Owner | Produces or consumes | Must not own |
|---|---|---|---|
| BIR view producer | `src/backend/bir/` semantic-view layer | Read-only producer, memory, publication, call, comparison, return, and control-flow facts derived from BIR | Frame layout, physical/value homes, moves, freshness, ABI placement, or destination authority |
| prepared fact producer | `src/backend/prealloc/` | Frame, value-home, move-bundle, freshness, prepared publication, aggregate stack-source, branch stack-load, call-plan, and destination-authority facts | Route status as executable authority or target instruction materialization |
| MIR consumer | common `src/backend/mir/` prepared-view/query layer | Stable named BIR facts while migration requires them, and fail-closed prepared facts for executable placement decisions | Re-running BIR route analysis or reconstructing prepared authority |
| target materializer | x86, AArch64, and RV64 MIR backends | Common MIR-facing facts plus target profile/ABI rules, producing target instructions or object intent | Target-local route indexes, fallback route inference, or changing shared authority |
| debug/proof artifact | printers, dumps, agreement checks, and tests | Named agreement, attribution, availability, and rejection evidence | Semantic selection, freshness, placement, or lowering authority |
| compatibility bridge | private route builders, route facades, transitional adapters, and legacy fixtures | Behavior-preserving forwarding while each consumer migrates | A durable public API, a full route record under a new name, or new route-numbered prepared state |

The first owner is decisive.  A target that observes a BIR fact remains a
consumer; it does not become the producer.  A route fact used while constructing
a prepared record is input evidence; the prepared producer owns the resulting
executable decision.

## Named BIR Semantic Contracts

These views expose only source-semantic answers.  Each query should return a
small record or explicit unavailable/ambiguous result keyed by function, block,
instruction, value, edge, or call identity as appropriate.

| Contract | Current evidence family | Narrow payload | First consumers |
|---|---|---|---|
| `BirProducerView` | Route 1 producer/value/materialization records | Produced value, producer instruction/block, same-block relationship, immediate constant or materialization availability | common MIR query during migration; memory, publication, call, and comparison view producers |
| `BirMemoryAccessView` | Route 3 memory/base/source records | Access identity and kind, address base, result or stored value, and same-block load/global source | prepared addressing/memory producer; temporary common MIR query |
| `BirPublicationView` | Routes 4 and 5 publication records | Current-block, block-entry, or CFG-edge identity; published value/source; predecessor/successor | prepared publication producer; named publication proof |
| `BirCallBoundaryView` | Route 6 call-use and result records | Call/callee identity, argument index and source kind/value, direct-global dependency, result value and lane | prepared call-plan producer; temporary call-query consumers |
| `BirComparisonView` | Route 7 comparison and branch-condition records | Comparison instruction, operand identities, predicate/condition, materialized condition, and branch use | prepared comparison/control producer; temporary AArch64 comparison consumer |
| `BirReturnView` | current return-chain/provenance dependencies, including Route 2/8-shaped records identified by the inventory | Return instruction, terminal returned value, next operand/provenance link, and chain completeness | prepared return/ABI producer or a temporary common MIR return query |
| `BirControlFlowView` | select-chain, branch, join, and Route 2/7/8 control relationships | Select producer/dependencies, branch condition identity, successor/predecessor edge, join source, and completeness | prepared control-flow producer; temporary common MIR control query |

`BirControlFlowView` is the umbrella for `BirSelectChainView` and branch/join
queries; `BirComparisonView` is separate because comparison operands and
predicate semantics are useful without granting publication or stack-load
authority.  `BirReturnView` is public only while a target-independent consumer
exists.  If return-chain traversal remains merely an old target helper, it
stays a private compatibility adapter instead.

BIR publication answers may identify a source-program publication.  They do
not select a prepared home, freshness proof, move, frame slot, or destination.
The call view describes BIR call relationships; `PreparedCallPlanLookups` owns
ABI resources and placement.  The comparison/control views describe uses;
prepared branch facts own executable branch transfers and stack loads.

## Named Prepared And Prealloc Contracts

Prepared contracts are status-rich, read-only MIR inputs.  A positive row is
emitted only after its inputs are present, consistent, and uniquely selected.

| Contract | Prepared producer and inputs | Required positive payload | MIR rule |
|---|---|---|---|
| `PreparedFrameLayoutView` | frame planning over prepared stack objects/slots | Function, frame size/alignment, slot and object ids, offset, size, alignment, fixed/dynamic state, frame-pointer policy | Reject missing, incomplete, dynamic-unsupported, or ambiguous slots |
| `PreparedValueHomeView` | value-location and regalloc/storage production | Value id, home kind, register or slot, offset, size, alignment, and materialization details | Never infer a home from producer/publication identity |
| `PreparedMoveBundleView` | move-bundle and move-resolution production | Cursor/edge, phase, authority kind, source/destination values and homes, selected move, cycle-temp and destination placement | Execute only a unique supported selected move |
| `PreparedPublicationView` | prepared control flow, homes, moves, and named BIR publication input | Edge/current-block identity, source/destination values and homes, selected move and publication status | Route agreement is optional proof, never freshness or move authority |
| `PreparedStackSourceView` | frame layout, homes, addressing, publication, and aggregate-copy planning | Slot/object, offset, size/alignment, aggregate lanes/copy width, ABI-layout reference, and scratch ownership | Reject incomplete or route-only aggregate stack evidence |
| `PreparedBranchStackLoadView` | prepared branch use plus home/frame/freshness selection | Branch cursor and role, value home, frame slot/object, selected freshness reference and status | A BIR comparison identifies the use but cannot authorize the load |
| `PreparedStackDestinationAuthorityView` | composition of frame, home, move, freshness, publication, aggregate-source, and branch-load facts | Destination value/home/storage, source value/home, selected move, selected freshness, relevant publication and stack-source authority | Lower only `Available`; reject missing, invalid, ambiguous, or route-only authority |

The existing `PreparedValueHome`, `PreparedMoveBundle`,
`PreparedMoveResolution`, `PreparedValueFreshnessAuthority`,
`PreparedAggregateStackSourceAuthority`, `PreparedBranchStackLoadAuthority`,
`PreparedStackLayout`, and `PreparedFramePlanFunction` are evidence-bearing
contacts for these views.  A follow-up may adapt them; it must not create a
parallel authority merely to obtain the proposed names.

## MIR And Target Handoff

Common MIR should expose the contracts through `PreparedMirCoreView`,
`PreparedMirFunctionView`, and small feature views.  The stable handoff is:

1. A BIR semantic producer answers an ownership-named source query.
2. Prealloc combines that answer with prepared control flow, homes, frame,
   moves, freshness, publication, and ABI plans and emits a positive or
   explicit negative prepared fact.
3. Common MIR binds the fact to one function/block/instruction cursor and
   rejects missing, mismatched, unsupported, or ambiguous input.
4. A target materializer chooses target instruction spelling and legal ABI
   realization without changing the selected source, home, move, freshness,
   publication, stack, or destination authority.

Temporary MIR consumers may use a named BIR view for source-semantic questions
while direct route queries are retired.  Placement questions must already use
prepared facts.  No target may rebuild Routes 1-8, manufacture a route index,
or fall back from a negative prepared result to a route record.

## Public And Private Header Boundary

The durable public boundary should be split by owner rather than by route:

- A small BIR semantic-view header family under `src/backend/bir/` declares
  read-only `BirProducerView`, `BirMemoryAccessView`, `BirPublicationView`,
  `BirCallBoundaryView`, `BirComparisonView`, `BirReturnView`, and
  `BirControlFlowView` records and queries.  It exposes stable ids and narrow
  results, not builder indexes or full route records.
- Prepared record headers under `src/backend/prealloc/` declare the fact
  payloads and producer-owned status.  They may accept named BIR facts as
  construction input, but public prepared records contain no `RouteN`,
  `RouteIndex`, `route_index`, or route-agreement authority fields.
- MIR public/internal handoff headers under `src/backend/mir/` expose only
  read-only core/function/feature views and cursor-bound queries.  Target
  headers depend on these views, not BIR route headers or unrestricted
  `PreparedBirModule` state.
- Current route builders, route indexes, prerequisite walkers, facade records,
  and route-to-named conversion helpers move behind private BIR implementation
  headers.  They may support adapters during migration but cannot be included
  by prealloc, MIR targets, or new tests.
- Compatibility and proof adapters use a clearly transitional private header.
  They return named agreement/proof rows and may retain route vocabulary only
  internally.  Debug headers may expose named proof records but never semantic
  builder indexes.

A compile-time alias, inheritance wrapper, accessor returning `const RouteN&`,
or aggregate that reproduces every `RouteN*` field fails this boundary even if
its type is ownership-named.  Public tests should construct the named input or
exercise the producer; direct route fixtures remain transitional compatibility
tests until their migrated consumer no longer needs them.

## Debug, Proof, And Compatibility Policy

Named proof rows may report producer agreement, publication agreement,
comparison agreement, call-source agreement, return-chain completeness, or
prepared-authority rejection.  They are observational.  Dumps, phase notes,
freshness summaries, source order, final assembly text, and route status do not
select semantic facts.  Tests follow the semantic consumer migration and must
not establish authority through expectation-only renames.

Private route builders can remain temporarily when they are the implementation
of a named BIR view.  Their retirement guard is that no public/prealloc/MIR/
target header or consumer mentions route vocabulary.  Route facade/status
records remain compatibility or proof and must shrink rather than grow.

## Positive Evidence Gate For Ideas 647 And 655

Ideas 647 and 655 remain parked until all of the following are demonstrated by
the prepared/prealloc producer, not inferred from route dumps or agreement:

1. A named producer emits a unique positive stack-destination authority row.
2. The row names destination value, destination home and storage kind, with
   complete register or frame-slot/object placement.
3. It names source value/home, the selected move bundle or move resolution,
   move authority kind, and the relevant edge/cursor.
4. It names one selected `PreparedValueFreshnessAuthority` (or successor) with
   matching use kind, source kind, proof kind, rank, and referenced fact.
5. Stack sources carry slot/object id, offset, size, alignment, and aggregate
   stack authority including copy width/lane/ABI/scratch evidence when needed.
6. Branch stack loads carry branch cursor and role, value home, frame
   slot/object, source freshness status, and the selected freshness authority.
7. The MIR view exposes the row as `Available` and fails closed for missing
   homes/moves/freshness, incomplete stack evidence, ambiguous fan-in, and
   route-only evidence.
8. A prepared-contract, prepared-MIR, object, object-runtime, or runtime proof
   exercises both the positive producer and at least one fail-closed case.

Route 4/5/7 records, `RouteIndex` validation, dumps, expectations, allowlists,
and agreement-only tests cannot satisfy any missing item.  If the producer
cannot emit the positive row, the next idea must repair that producer seam;
ideas 647 and 655 must not resume at the MIR consumer.

## Step 3 Handoff Constraints

The follow-up queue should split BIR view production, prepared fact production,
common MIR consumer migration, each target materializer migration, route
quarantine, and trailing debug/test vocabulary cleanup into single-owner ideas.
Every idea must name its first consumer and proof surface.  Producer ideas may
retain private route algorithms; consumer ideas may not expand them.  Stack-
authority work is ordered after positive prepared producer evidence and before
ideas 647 or 655.
