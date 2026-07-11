# BIR MIR Contract Abstraction Umbrella

Status: Closed
Type: Umbrella triage and follow-up idea generator
After: `ideas/closed/694_bir_route_index_retirement_umbrella.md`
Parent: `ideas/closed/694_bir_route_index_retirement_umbrella.md`
Handoff Directory: `docs/bir_mir_contract_abstraction/`
Related:
- `ideas/closed/683_prepared_mir_view_contract_research.md`
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
- `ideas/closed/693_bir_route_index_retirement_research.md`
- `ideas/closed/694_bir_route_index_retirement_umbrella.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `docs/prepared_mir_view_contract_research/`
- `docs/bir_route_index_retirement_research/`
- `docs/bir_route_index_retirement/`
- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/`

## Goal

Use the route-retirement and prepared-MIR-view evidence to classify the
remaining BIR-to-MIR contract leaks and generate ordered follow-up ideas that
make MIR and prealloc consume named handoff views instead of route-numbered BIR
analysis APIs.

## Why This Exists

The route-retirement batch cleaned up several local proof and vocabulary
surfaces, but it did not make the numbered `route*` layer disappear. The
reason is architectural: `route*` is still acting as a cross-layer contract.
MIR and prealloc consumers can still observe how BIR discovered producers,
memory accesses, publications, calls, comparisons, and branch or stack facts,
instead of consuming a narrow semantic handoff.

As long as downstream code can call or name `route1`, `route2`, `route3`,
`route4`, `route5`, `route6`, `route7`, `route8`, `RouteIndex`, or related
facade APIs, those routes remain public architecture rather than private BIR
implementation history. Another rename-only cleanup would leave the same
failure mode behind new names.

This umbrella exists to turn the observation into an explicit contract plan:
BIR may build named semantic views internally, prealloc may prepare target
facts from those views, and MIR must consume prepared/MIR-facing handoff
objects without rediscovering route analysis.

## Current Evidence

- `ideas/closed/694_bir_route_index_retirement_umbrella.md` closed after
  generating follow-up ideas, but the completed follow-ups mostly handled
  facade compatibility, proof vocabulary, selected consumer slices, stack view
  prerequisites, and dump policy.
- A fresh source scan still shows route-numbered files and APIs under
  `src/backend/bir/`, including `bir_route1.cpp` through `bir_route8.cpp`,
  `bir_route_facade.cpp`, `bir_route_index.hpp`, and
  `bir_route_index_prereqs.hpp`.
- A fresh route vocabulary scan still reports direct hits under
  `src/backend/mir/query.cpp`, `src/backend/prealloc/`,
  target MIR code, and backend tests.
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
  added a first prepared MIR view and an x86 dependency guard, but it did not
  abstract the full BIR-to-MIR contract.
- Parked ideas 647 and 655 show that residual stack-destination authority
  should not be inferred by MIR from route records. They need explicit
  prepared/prealloc producer facts before resuming.

## In Scope

- Create `docs/bir_mir_contract_abstraction/` as the durable handoff directory.
- Classify every remaining route-numbered dependency by first owning layer:
  BIR semantic view producer, prealloc/prepared fact producer, MIR consumer,
  target-specific materializer, debug/proof artifact, or compatibility-only
  bridge.
- Define the named BIR/prepared-to-MIR handoff views needed to replace route
  records as cross-layer contracts.
- Generate ordered follow-up ideas under `ideas/open/` for the required
  contract work.
- Require each follow-up idea to name its owning layer, first migrated
  consumer, proof surface, and route-vocabulary retirement guard.
- Define where stack, frame, value-home, move-bundle, branch stack-load, and
  destination-authority analysis belongs before ideas 647 or 655 can resume.
- Keep route-numbered builders private during migration only when they are not
  visible to MIR, prealloc consumers, prepared views, target materializers, or
  public tests.

## Out Of Scope

- Implementing BIR, prealloc, MIR, or target code changes inside this umbrella.
- Rewriting all of BIR-to-prealloc or BIR-to-MIR in one follow-up idea.
- Reactivating ideas 647 or 655 before explicit prepared/prealloc stack
  authority producer evidence exists.
- Treating route dump rows, debug facts, freshness proofs, source order,
  testcase identity, or final assembly as semantic authority.
- Test expectation rewrites, unsupported-marker edits, allowlist edits,
  timeout/accounting changes, runtime behavior changes, or baseline-policy
  changes as proof of contract cleanup.

## Priority Model

Order generated follow-up ideas by contract ownership rather than by route
number:

1. BIR semantic handoff view shape and public/private header boundary.
2. MIR/prealloc consumer migration away from direct route queries.
3. Prepared view expansion for facts that MIR legitimately consumes.
4. Target MIR materializer cleanup for route vocabulary that leaked through
   backend-specific lowering.
5. Test and debug vocabulary cleanup after the semantic consumers no longer
   depend on route names.
6. Route file rename, deletion, or quarantine only after public consumers are
   gone.
7. Residual stack-destination authority revisit only after explicit prepared
   producer evidence exists.

Prefer a follow-up that removes one public route dependency family from a real
consumer over a broad helper rename. A route-numbered implementation may remain
temporarily if the public contract and tests no longer expose it.

## Required Follow-Up Ideas

Generate at least these follow-up families unless the handoff docs prove a
better split:

- `BIR semantic handoff view contract`: owning layer BIR; defines named
  producer, memory, publication, call, comparison, return, and control-flow
  value views without route-numbered public types.
- `MIR query contract migration`: owning layer MIR query interface; migrates
  `src/backend/mir/query.cpp` and direct MIR consumers from route APIs to
  prepared or named handoff views.
- `Prealloc prepared fact boundary migration`: owning layer prealloc/prepared;
  turns BIR views into prepared facts without exposing route records as
  authority.
- `Target MIR route vocabulary cleanup`: owning layer target MIR backends;
  removes route-numbered names from x86, RV64, AArch64 materializers once the
  common query contract is available.
- `Stack and destination authority contract gate`: owning layer
  prepared/prealloc producer evidence; defines the minimum positive facts
  required before ideas 647 and 655 can resume.
- `Route implementation quarantine and file retirement`: owning layer BIR
  internals; renames, deletes, or quarantines `bir_route*.cpp` and
  route-index headers after public consumers no longer rely on them.
- `Route vocabulary test cleanup`: owning layer backend tests and debug proof;
  migrates tests from route-numbered fixtures to named contract fixtures after
  semantic consumers have moved.

## Acceptance Criteria

- `docs/bir_mir_contract_abstraction/` contains a current dependency inventory,
  contract ownership classification, named handoff view proposal, migration
  sequence, and ordered follow-up plan.
- The inventory is based on a fresh scan of route vocabulary across
  `src/backend/bir`, `src/backend/prealloc`, `src/backend/mir`, target MIR
  backends, and backend tests.
- The documents explicitly distinguish semantic codegen inputs from
  debug/proof/compatibility artifacts.
- Generated follow-up ideas appear under `ideas/open/` and are ordered by the
  priority model above.
- Each generated follow-up idea names its first owning layer, first migrated
  consumer, proof surface, and route-vocabulary retirement guard.
- The follow-up plan includes a concrete guard such as:
  `rg "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index"`.
- The plan states where stack/frame/value-home/move-bundle/destination
  authority analysis happens and what form MIR may consume.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, default harness contracts,
  or baseline policy.

## Closure Note Requirements

The closure note must state which scans, docs, and closed ideas were used;
which handoff documents were written; which follow-up ideas were generated;
how route-vocabulary guards are expected to shrink; which route-numbered APIs
remain private compatibility; and what explicit prepared/prealloc stack
authority evidence is required before ideas 647 and 655 can resume.

## Closure Note

Closed on 2026-07-11 after the four-step umbrella runbook completed and the
matching close-time regression guard passed with 2/2 tests before and after.
Because this umbrella changed only planning and documentation artifacts, the
guard used the maintenance policy permitting an unchanged pass count.

The audit used the fresh broad scan
`rg -n "route[1-8]_|Route[1-8]|bir_route[1-8]|RouteIndex|route_index"
src/backend/bir src/backend/prealloc src/backend/mir tests`, the four documents
under `docs/bir_mir_contract_abstraction/`, and closed ideas 683, 684, 693,
and 694. The handoff documents are the current dependency inventory, ownership
and named-contract proposal, ordered follow-up queue, and handoff audit and
closure evidence.

The umbrella generated ideas 704 through 712: BIR semantic views, the prepared
fact boundary, common MIR query migration, the positive stack-destination
authority gate, x86/AArch64/RV64 target migrations, BIR route quarantine, and
trailing debug/test vocabulary cleanup. The guard is expected to shrink first
through producer and common-consumer work in 704-706, then target work in
708-710, private implementation quarantine in 711, and final proof vocabulary
cleanup in 712.

Only BIR-private builders, prerequisite walkers, indexes, facades, and narrow
route-to-named adapters may remain as temporary route-numbered compatibility.
They may not expose complete route records to prealloc, MIR, targets, prepared
records, public headers, or public tests, and idea 711 owns their shrink or
retirement.

Ideas 647 and 655 remain parked until idea 707 is accepted with a unique,
complete positive prepared producer row containing destination and source
homes, selected move and freshness, publication, and relevant stack-source or
branch-load evidence; cursor-bound MIR consumption must accept only
`Available`, and positive plus fail-closed negative proof must pass. Merely
creating idea 707 or observing route agreement or dumps does not satisfy this
gate.

## Reviewer Reject Signals

- Reject direct implementation inside the umbrella idea.
- Reject output that only renames `route*` files, helpers, or tests without
  removing route-numbered APIs from cross-layer contracts.
- Reject a named handoff view that simply exposes the old `RouteN*` records or
  full BIR/prepared internals under a new wrapper.
- Reject follow-up ideas that mix BIR view definition, prealloc prepared fact
  production, MIR consumer migration, target materialization, and test cleanup
  into one broad rewrite.
- Reject MIR-side rediscovery of stack destination authority from raw route
  records instead of explicit prepared/prealloc producer facts.
- Reject using route dumps, debug/proof records, freshness observations,
  source order, final assembly, or testcase identity as semantic authority.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, timeout/accounting changes, or weaker proof
  commands as progress.
- Reject closure if route-numbered vocabulary remains classified only as
  "later cleanup" without an owning follow-up idea and acceptance guard.
