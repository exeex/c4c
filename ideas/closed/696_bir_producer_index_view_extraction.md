# BIR Producer Index View Extraction

Status: Closed
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 2

## Goal

Extract a named BIR producer view for same-block scalar producers and immediate
materialization availability without making Route 1 or Route 2 route-numbered
APIs public architecture.

## Why This Exists

The handoff defers broader producer and control route extraction until each
family has one owning layer and one first migration surface. Route 1 producer
identity and Route 2 select/direct-global dependencies are substrates for
later materialization proof, but the first packet must be a narrow BIR view
extraction rather than a prealloc or target lowering rewrite.

## Owned Files

- `src/backend/bir/` producer, select-chain, and control-value route builders,
  named BIR view headers, and proof adapters.
- Focused BIR or backend tests for the named producer view contract only.

## First Owning Layer

BIR semantic producer and control-value view layer.

## First Producer Migration

Publish a `BirProducerView` or equivalent named view over existing Route 1
producer facts, with Route 2 select-chain or direct-global facts included only
when needed for that producer contract.

## Proof Surface

Build plus focused BIR/backend proof that the named producer view returns the
same producer identity and materialization availability as the old route
builder for one low-risk consumer. Use prepared, MIR, object, or runtime proof
only if the packet changes executable behavior.

## Numbered Route APIs Kept Private Compatibility

- `Route1ProducerIndex`
- `Route2SelectChainValueIndex`

These may remain private implementation details behind `BirProducerView` or a
named control-value proof view. They must not become prepared source
freshness, value-home, stack destination, move-bundle, publication, or MIR
authority.

## In Scope

- Add a named producer view wrapper over existing Route 1 facts.
- Keep select/direct-global Route 2 facts scoped to producer proof when they
  are required by the first migration.
- Preserve current route-builder behavior and lifetimes.
- Add explicit missing or unavailable view states if the old route result is
  absent.

## Out Of Scope

- Memory, publication, call, return, dump-policy, or stack-authority cleanup.
- Prealloc consumer migration beyond the one proof reader needed to prove the
  named producer view.
- Target-specific materialization policy changes.

## Acceptance Criteria

- A named producer view owns the first producer lookup surface.
- Route 1 and Route 2 remain private compatibility internals.
- The first migrated proof surface is behavior-preserving or has stronger
  executable proof if behavior changes.
- Missing producer evidence remains explicit instead of falling back to route
  dumps or testcase identity.

## Reviewer Reject Signals

- Reject named-case shortcuts for a single failing test.
- Reject exposing `Route1ProducerIndex` or `Route2SelectChainValueIndex` as a
  new public API under different names.
- Reject using producer lookup as stack destination, publication, value-home,
  move-bundle, or freshness authority.
- Reject prealloc or target lowering rewrites before the BIR producer view is
  proven.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  helper renames claimed as producer-view progress.

## Completion Note

Closed after commit `4834bc655` completed the Route 1-only named producer-view
adapter and first proof-reader migration. The slice added a
`bir::BirProducerView` compatibility surface over existing Route 1 producer
facts with `bir::make_bir_producer_view`,
`bir::find_same_block_scalar_producer`, and
`bir::find_materialization_availability`, then migrated the selected `%sum`
proof reader in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
through the named view.

Route 2 select-chain and direct-global facts were explicitly deferred because
the first named producer-view proof did not require them. No prealloc
authority, MIR lowering, target materialization, memory/publication view,
call/return view, dump vocabulary, expectation, unsupported-marker, allowlist,
timeout, or runtime contract changed.

Close proof:

- corrected focused before/after proof:
  `cmake --build --preset default && build/tests/backend/bir/backend_prepared_lookup_helper_test`
  passed before and after the slice
- full baseline candidate accepted separately: 3333 passed / 0 failed

The focused proof logs are build-plus-binary logs rather than CTest summary
logs, so the CTest regression checker cannot parse them directly. The accepted
full baseline candidate is the close-scope regression evidence for this
behavior-preserving adapter packet.

Remaining Route 2 producer/control extraction, memory/publication extraction,
call/return extraction, prealloc consumer migration, dump cleanup, and stack
authority prerequisite work belongs to the separate follow-up ideas already
parked under `ideas/open/`, not to this closed Route 1-only producer-view
packet.
