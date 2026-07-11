# Prepared Fact Boundary From Named BIR Views

Status: Closed
Type: prealloc/prepared contract implementation
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: `ideas/open/704_bir_semantic_handoff_views.md`

## First Owner And Scope

First owning layer: `src/backend/prealloc/`.  Convert named BIR source facts
into existing or narrowly extended prepared publication, call-plan, lookup,
home, frame, move, and control facts without persisting route records as
authority.

First migrated consumer: `src/backend/prealloc/publication_plans.cpp`, followed
within this owner by call plans and lookup attribution.

## Dependencies And Proof

- Depends on idea 704's named BIR views.
- Proof surface: prepared block-entry/publication, call-contract, lookup,
  printer, and store-source tests, including explicit missing/ambiguous input.

## Retirement Guard

The umbrella guard must shrink in `src/backend/prealloc`.  Route agreement may
remain temporary observational proof, but prepared selection and public record
headers must not branch on or expose it.

## Acceptance Criteria

- Prepared production accepts narrow BIR facts and owns executable home, move,
  freshness, publication, frame, call-plan, and control decisions.
- Negative or incomplete inputs remain explicit and fail closed.
- No parallel authority is invented merely to adopt proposed contract names.

## Reviewer Reject Signals

- Route status/agreement becomes freshness, destination, or move authority.
- A prepared record embeds a route record or route-numbered authority field.
- Expectations are rewritten without migrating the producer seam.

## Bounded Consumer Handback

Idea 713 closed the current-block result-consumption dependency at commit
`1394423de` with final accepted review and 329/329 backend proof. The
prealloc/function-context owner constructs and attaches the complete stable-key
routing query; AArch64 consumes only that query and fails closed when ownership
or stable identity is absent. Route 5 remains diagnostic-only, all-applicable
semantic ambiguity is preserved, and no target reconstruction or function-wide
publication/transfer scan survives in the bounded consumer. Resume at Step
2.3b.3 to retire the now-obsolete Route 5 public compatibility payload without
replanning the accepted consumer authority.

## Completion

Closed after the prepared publication, call-plan, and lookup producer seams
were migrated to named BIR inputs with explicit fail-closed boundary states.
Public prepared records no longer expose route-numbered executable authority;
remaining route/agreement vocabulary is private observational compatibility,
debug/proof vocabulary, or deferred target/materialization scope. The final
boundary audit and matching backend regression guard passed 329/329 tests.
