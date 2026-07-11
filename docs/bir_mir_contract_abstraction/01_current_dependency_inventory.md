# Current Route-Numbered Dependency Inventory

This is the Step 1 baseline for the BIR-to-MIR contract-abstraction umbrella.
It records vocabulary dependencies, not a claim that every textual hit is an
executable dependency.  The classification below reconciles the current tree
with the prepared-MIR-view and route-index-retirement research.

## Reproduction

The delegated command named both `test` and `tests`, but this checkout has no
`test/` directory.  The exact successful scan was:

```sh
rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index" src/backend/bir src/backend/prealloc src/backend/mir tests
```

Run on 2026-07-11, it produced 5,339 matching lines in 56 unique files: 13
BIR files, 6 prealloc files, 20 MIR/target files, and 17 test files.  This is a
deliberately broad vocabulary guard; counts can include several matched tokens
on one logical dependency and declarations used by multiple consumers.

| Vocabulary | BIR files | prealloc files | MIR/target files | test files |
|---|---:|---:|---:|---:|
| Route 1 | 13 | 0 | 6 | 6 |
| Route 2 | 3 | 0 | 3 | 3 |
| Route 3 | 4 | 0 | 9 | 7 |
| Route 4 | 8 | 2 | 3 | 6 |
| Route 5 | 2 | 3 | 7 | 6 |
| Route 6 | 2 | 1 | 8 | 6 |
| Route 7 | 6 | 0 | 1 | 3 |
| Route 8 | 2 | 0 | 1 | 2 |
| `RouteIndex` / `route_index` | 5 | 1 | 2 | 5 |

## Classification Key

- **Semantic**: currently read to select, construct, or validate lowering data;
  it is a real migration dependency even when prepared data should ultimately
  own the decision.
- **Debug/proof**: agreement, attribution, diagnostics, dumps, or tests.  It
  may prove a producer but is not executable authority.
- **Compatibility**: a public forwarding facade, old type/header boundary, or
  transitional adapter.  Renaming it alone does not retire the dependency.

“Current producer” names the implementation that supplies the route-numbered
fact today.  It does not endorse that producer as the future contract owner.

## BIR Producers and Public Boundary

| Dependency family | Current producer | Current consumers | Class |
|---|---|---|---|
| Route 1 same-block producer/value identity and immediate materialization | `bir_route1.cpp`, declarations and builders in `bir.hpp`/`bir.cpp` | generic MIR query; x86 and AArch64 scalar, call, dispatch, and materialization paths; route composition | Semantic |
| Route 2 return-chain provenance | `bir_route2.cpp` over Route 1 producer facts | generic MIR query and x86/AArch64 return-oriented lowering | Semantic |
| Route 3 memory access/base/source identity | `bir_route3_memory.cpp` over Route 1 facts | generic MIR query; x86/AArch64 memory paths; RV64 prepared edge-publication agreement | Semantic where it supplies memory identity; debug/proof where it only checks a prepared source |
| Route 4 publication availability and block-entry/current-block references | `bir_route4_publication.cpp` | prealloc block-entry attribution; generic MIR query; AArch64 call-boundary fallback/validation | Semantic BIR publication fact; debug/proof after prepared publication already exists |
| Route 5 CFG-edge publication and join-source identity | `bir_route5_publication.cpp` over Routes 1/3/4 | prealloc publication facts and printer; generic MIR query; x86/RV64 edge publication; AArch64 join routing | Semantic BIR edge fact; debug/proof for stored `route5_*_agrees/status` annotations |
| Route 6 call-use, argument-source, result-source, and direct-global dependencies | `bir_route6_call_publication.cpp` over producer/memory/publication facts | prealloc call plans; generic MIR query; x86/AArch64 call lowering and AArch64 dispatch | Semantic |
| Route 7 comparison operands/condition/control relationships | `bir_route7_comparison.cpp` | generic MIR query and AArch64 comparison lowering | Semantic BIR comparison fact or target validation; route-index relationship/status records are debug/proof |
| Route 8 select/materialization relationships | `bir_route8.cpp` | generic MIR query and AArch64 select materialization | Semantic |
| Route-index facade, reference validation, prereqs, and status vocabulary | `bir_route_facade.cpp`, `bir_route_index.hpp`, `bir_route_index_prereqs.hpp` over Routes 4/7 | prepared lookup attribution and limited MIR validation/query paths | Compatibility; validation/status output is debug/proof |
| Route-numbered declarations exposed through `bir.hpp` and route-index headers | BIR public headers | every prealloc, MIR, target, and test include/caller above | Compatibility boundary carrying semantic record types |

The BIR builders may remain private during migration, but the current header
and direct-call surface makes them cross-layer architecture today.  A named
wrapper that still exposes a complete `RouteN*` record remains compatibility,
not a completed semantic handoff.

## Prealloc and Prepared Dependencies

| Consumer | Current producer | Dependency and classification |
|---|---|---|
| `prepared_lookups.cpp` | Route 4 plus already-built prepared block-entry publication | Route 4/index reference agreement and attribution are **debug/proof**. `PreparedBlockEntryPublication`, its destination home, and move resolution remain semantic prepared authority. |
| `publication_plans.cpp/.hpp` | Route 5 plus prepared edge/current-block publication, value homes, moves, and freshness | Raw Route 5 edge identity can be a **semantic BIR input** while producing a named fact. Persisted `route5_join_source`, status, and agreement are **debug/proof**, not freshness or destination authority. |
| `value_locations.hpp` | prepared value-home and publication production, annotated from Route 4 | Route 4 block-entry fields are **debug/proof** attribution. The prepared home is semantic authority. |
| `call_plans.cpp` | Route 6 and prepared call-plan construction | Direct route call-source lookup is a **semantic migration dependency**; the durable boundary should be a named prepared call fact. |
| `prepared_printer/select_chains.cpp` | prepared records | Route 5 status/agreement labels are **debug/proof** only. |

No positive prepared stack-destination producer is established merely by these
hits.  Frame layout, value homes, move bundles, selected freshness, aggregate
stack-source authority, branch stack-load authority, and edge publication are
prepared-owned facts; route agreement cannot substitute for them.

## MIR and Target Consumers

| Layer / consumer | Current producer | Dependency and classification |
|---|---|---|
| Common `mir/query.cpp` and `query.hpp` | direct Routes 1-8 and route-index/BIR records | Broad **semantic** cross-layer dependency. This is the clearest common consumer to migrate to named BIR or prepared views. |
| x86 module lowering | direct Routes 1-6 plus prepared structures | **Semantic** producer, memory, publication, and call queries; agreement/debug paths remain **debug/proof**. |
| x86 debug and public route-debug structures | target lowering state and route records | **Debug/proof** and transitional **compatibility** vocabulary, not producer equivalence. |
| AArch64 calls, dispatch, ALU, globals, comparisons, and select materialization | direct Routes 1-8, sometimes compared with prepared facts | **Semantic** where route facts drive or fall back in lowering; **debug/proof** where they only establish agreement with an available prepared fact. Target-local recreation of Route 4 indexes remains a semantic migration dependency, not prepared authority. |
| RV64 prepared edge-publication emission | prepared publication plus Routes 3/5 agreement records | Prepared publication is semantic authority; Route 3/5 source agreement fields are **debug/proof/compatibility** unless lowering branches on their absence, which is a migration dependency to eliminate. |
| RV64 object-emission dump | prepared emission intent | `intent_route5_*` / `intent_route3_*` text is **debug/proof** only. |

The prepared-MIR-view model remains the intended target: MIR consumes stable
prepared core/feature views, and producer identity, route names, notes, phase
strings, and debug summaries do not affect lowering.  The current scan proves
that the first prepared view and its x86 boundary guard did not yet establish
that condition across common query, AArch64, or RV64.

## Backend Test and Proof Surface

The 17 matching test files fall into three dependency groups:

| Group | Representative current consumers | Current producer | Class |
|---|---|---|---|
| Direct route API/unit fixtures | `backend_prepared_lookup_helper_test.cpp` and route/prealloc BIR tests | public `RouteN*` builders, records, facade, and route-index headers | **Compatibility** plus **debug/proof**; tests keep public vocabulary alive but do not authorize codegen |
| Prepared contract/agreement tests | prealloc block-entry, prepared printer, frame/stack/call, store-source, scalar/memory operand tests | prepared records annotated or validated with Routes 3-6 | Prepared facts are semantic; route status/agreement expectations are **debug/proof** |
| Target handoff/lowering tests | x86 handoff/debug and AArch64 instruction, branch, join, call-boundary tests | target consumers of route facts and/or prepared records | **Semantic proof** when behavior depends on a direct route query; route labels and dump rows alone are **debug/proof** |

These tests should migrate after their semantic consumer.  Expectation-only
renames cannot demonstrate contract retirement.

## Reconciliation and Stale Prior Evidence

The current scan confirms the important ownership conclusions of the prior
research: prepared facts own executable frame, home, move, freshness,
publication, and destination authority; route records can describe BIR facts
or provide agreement evidence; route-index status and dumps are not authority.

It also makes these earlier snapshots stale or incomplete as a current
inventory:

1. The route-retirement digest's narrow Route 4/Route 7 facade description is
   still true of the facade itself, but it is not a description of the whole
   public contract. Direct Routes 1-8 remain visible to common MIR query and
   target consumers.
2. The prepared-MIR-view research described MIR entry points receiving large
   prepared structures and proposed a stable adapter boundary. A first view
   and x86 guard now exist, but the scan still finds 20 MIR/target files with
   route vocabulary, especially `mir/query.cpp`, AArch64, and RV64 agreement
   code. Treat the proposal as direction, not evidence of completed migration.
3. Route 4 block-entry attribution and stored Route 5 agreement were already
   classified as proof residue. Their continued prealloc and printer hits do
   not establish new semantic authority and must not be used to resume stack
   destination work.
4. Earlier route-retirement follow-ups removed selected facade/proof vocabulary
   but did not retire the underlying route-numbered builders, public record
   types, direct target calls, or test fixtures. The 5,339-line baseline is the
   current retirement guard.

## Step 2 Inputs

The next contract pass must distinguish: named BIR semantic producers for
producer, memory, publication, call, comparison, return, control-flow, and
select facts; prepared producers for frame, value-home, move-bundle,
freshness, publication, branch stack-load, and destination authority; common
MIR-facing views; target materializers; and trailing proof/compatibility
adapters. Ideas 647 and 655 remain parked until an explicit prepared producer
emits positive, unique stack-destination authority with destination/source
homes, selected move and freshness, relevant publication/stack facts, and a
fail-closed MIR view.
