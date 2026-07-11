# Handoff Audit And Closure Evidence

This Step 4 audit reconciles the current route-vocabulary guard with the
inventory, named contracts, ordered queue, and generated follow-up ideas.  It
prepares evidence for plan-owner review; it does not implement a follow-up,
close the umbrella, or claim that route retirement has already occurred.

## Audit Basis

The current source guard is:

```sh
rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index" \
  src/backend/bir src/backend/prealloc src/backend/mir tests
```

On 2026-07-11 it still finds the 56 files classified in
`01_current_dependency_inventory.md`: 13 BIR, 6 prealloc, 20 common/target
MIR, and 17 test files.  The audit also used the source umbrella, the prepared
MIR view research and implementation recorded by closed ideas 683 and 684,
and the route-index research and umbrella recorded by closed ideas 693 and
694.  Those earlier artifacts establish the distinction between semantic
prepared authority and route agreement/debug evidence; the fresh inventory,
not their older hit counts, is the current source baseline.

The durable handoff set is:

- `01_current_dependency_inventory.md`: current hits and semantic,
  debug/proof, and compatibility classification.
- `02_ownership_and_named_handoff_contracts.md`: named BIR, prepared, MIR, and
  target boundaries plus the stack-authority resume gate.
- `03_ordered_followup_queue.md`: dependency order and generated-idea map.
- this audit: complete guard-family ownership and closure-note evidence.

## Guard-Hit Ownership And Expected Shrink

Every current hit belongs to one of the following dependency families.  The
owner column is the first idea responsible for removing the cross-layer
dependency; later owners remove target, implementation, or proof residue.

| Current guard-hit family | Contract destination | Shrink or retirement owner |
|---|---|---|
| Route 1 producer/value/materialization | `BirProducerView`, then named prepared/MIR facts | 704 defines the seam; 705/706 remove prepared/common consumption; 708/709 remove target use; 711 retires or quarantines builders; 712 removes fixtures/labels |
| Route 2 return-chain provenance | `BirReturnView` when a target-independent consumer remains; otherwise private compatibility | 704 classifies/defines the narrow view; 706 migrates common queries; 708/709 migrate target use; 711 confines or retires the old traversal; 712 cleans proof vocabulary |
| Route 3 memory/base/source identity | `BirMemoryAccessView` feeding prepared memory/address facts | 704, then 705/706, target owners 708-710, implementation owner 711, proof owner 712 |
| Route 4 current-block/block-entry publication | `BirPublicationView` plus `PreparedPublicationView` and prepared home attribution | 704/705 remove executable and attribution boundary leaks; 706 and 709 remove MIR/AArch64 fallback; 711/712 retire implementation and proof residue |
| Route 5 edge publication/join source | `BirPublicationView` feeding prepared publication, homes, moves, and freshness | 704/705 own production; 706 owns common queries; 708-710 own target dependencies; 711 owns builders; 712 owns status/agreement labels and fixtures |
| Route 6 call use/argument/result/global facts | `BirCallBoundaryView` feeding prepared call plans | 704/705 own the producer boundary; 706 owns common queries; 708/709 own x86/AArch64 consumers; 711/712 own implementation and proof residue |
| Route 7 comparisons/conditions/control relationships | `BirComparisonView` and `BirControlFlowView`, with prepared branch authority | 704 defines facts; 705 produces prepared decisions; 706 migrates common query; 709 migrates AArch64; 711 retires indexes/builders; 712 removes proof vocabulary |
| Route 8 select/materialization relationships | `BirControlFlowView`/select-chain and, where applicable, `BirReturnView` | 704, 706, 709, 711, then 712; no target-local reconstruction is permitted |
| `RouteIndex`, `route_index`, facade, prereq, and status records | no durable public equivalent; named narrow result/status only | 704 removes the public header need; 705/706 remove prepared/MIR observation; 711 quarantines or deletes the mechanism; 712 removes direct fixtures |
| prealloc hits in lookups, publication, call plans, locations, and printer | named BIR inputs and prepared-owned frame/home/move/freshness/publication/call facts | 705 owns executable records and adapters; 706 removes MIR exposure; 710 removes RV64 agreement coupling; 712 removes trailing labels/tests |
| common `mir/query.*` direct route queries | `PreparedMirCoreView`, `PreparedMirFunctionView`, and small named feature views | 706 is the single first consumer owner; target ideas may rely on it but must not recreate its analysis |
| x86 module/debug/header hits | common named MIR handoff; observational named debug rows | 708 removes materializer use; 712 removes trailing debug and public-fixture vocabulary |
| AArch64 dispatch/call/ALU/global/comparison/select hits | common named MIR/prepared facts | 709 owns all AArch64 fallback, local-index, and route-driven materialization dependencies; 712 owns trailing proof text |
| RV64 prepared edge publication and object dump hits | prepared publication as sole semantic authority | 710 removes route agreement from control decisions; 712 removes `intent_route*` and proof residue |
| direct route API, prepared agreement, and target handoff test hits | named contract producer/consumer tests | each semantic test follows 704-710; 712 owns final direct fixtures, CMake names, labels, and zero-public-vocabulary proof |

No family is deferred as generic later cleanup.  Ideas 704-712 name the first
owner, first migrated consumer, proof surface, acceptance criteria, retirement
guard, and reject signals for their assigned family.

## Permitted Private Compatibility State

The only route-numbered APIs permitted to remain temporarily are BIR-private
builders, prerequisite walkers, indexes, facades, or route-to-named adapters
that implement a narrow named BIR view.  This permission is conditional:

- no prealloc, common MIR, target, public header, or new/public test may include
  the private route boundary or receive a complete `RouteN*` record;
- the adapter may not reproduce a full route record under a named wrapper or
  create route-numbered prepared state;
- route status and agreement remain observational and cannot select homes,
  moves, freshness, publication, stack placement, or lowering;
- the private surface must shrink under 711's include/dependency guard and may
  not grow while consumers migrate; and
- direct legacy fixtures are transitional proof owned for final removal by
  712, not a reason to keep the route API public.

If `BirReturnView` has no target-independent consumer after migration, its old
Route 2/8-shaped traversal is also only private compatibility and is owned by
711.  There is no permitted private-compatibility exception in prealloc, MIR,
target materializers, prepared records, or public tests.

## Contract, Ordering, And Authority Reconciliation

The documents and ideas use one consistent contract vocabulary:
`BirProducerView`, `BirMemoryAccessView`, `BirPublicationView`,
`BirCallBoundaryView`, `BirComparisonView`, `BirReturnView`, and
`BirControlFlowView` are BIR source-semantic views.  Prepared ownership covers
`PreparedFrameLayoutView`, `PreparedValueHomeView`, `PreparedMoveBundleView`,
`PreparedPublicationView`, `PreparedStackSourceView`,
`PreparedBranchStackLoadView`, and
`PreparedStackDestinationAuthorityView`.  Common MIR exposes these through
core/function/feature views and targets only materialize them.

The dependency order is 704 -> 705 -> 706, with 707 providing a distinct
positive prepared stack-authority gate.  Target ideas 708-710 are sibling
consumers after their named common dependencies, not route-number ordering.
Quarantine 711 follows all semantic consumers, and proof/test cleanup 712
trails consumer and implementation retirement.

Ideas 647 and 655 remain parked.  They may resume only after 707 is accepted
with a unique positive prepared producer row containing complete destination
and source homes, selected move, selected freshness, publication and relevant
stack-source/branch-load evidence, exposed as `Available` through a
cursor-bound MIR view.  Proof must exercise the positive producer and a
fail-closed missing/invalid/ambiguous or route-only case.  Merely creating or
starting 707, obtaining route agreement, or observing dumps does not pass the
gate; lifecycle review must also decide whether 647/655 remain necessary.

## Closure-Note Evidence For Plan-Owner Review

- Scans used: the fresh broad route-vocabulary guard above, reconciled against
  the 56-file inventory; the guard is expected to shrink through 704-706,
  target siblings 708-710, private quarantine 711, and final proof cleanup 712.
- Documents written: the four files in this handoff directory listed under
  Audit Basis.
- Closed-idea evidence used: 683/684 for prepared-MIR view authority and the
  first boundary implementation; 693/694 for route-index research, residual
  vocabulary, and the need for ownership-based follow-ups.
- Follow-up ideas generated: 704 BIR views, 705 prepared boundary, 706 common
  MIR query migration, 707 positive stack-authority gate, 708 x86, 709
  AArch64, 710 RV64, 711 BIR quarantine, and 712 debug/test cleanup.
- Private compatibility allowed: only the conditional BIR-private mechanisms
  described above; all cross-layer and public route APIs have an explicit
  removal owner.
- Stack-authority resume evidence: the complete accepted 707 positive row,
  fail-closed MIR consumption, and positive plus negative proof described in
  the preceding section are required before 647 or 655 can resume.

This evidence satisfies the umbrella handoff audit, but formal closure still
belongs to plan-owner review and regression-guard handling.
