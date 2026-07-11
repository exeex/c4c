# Route Fact Test And Dump Contract Cleanup

Status: Closed
Type: Implementation
Parent: `ideas/open/694_bir_route_index_retirement_umbrella.md`
Handoff:
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
Queue Order: 7
Depends On:
- named proof surfaces for any dump rows being renamed

## Goal

Retire or gate transitional route-numbered dump and test vocabulary only after
the named BIR or prepared proof surface exists.

## Why This Exists

The handoff requires route dump spelling to follow semantic migration, not lead
it. Default CTest should continue proving connected MIR/object/runtime paths
where applicable, and route-view tests should be limited to named BIR semantic
views or temporary compatibility adapters with a planned rewrite point.

## Owned Files

- Backend test files and CMake wiring that own route fact dump expectations.
- Diagnostic printers for route proof vocabulary when the named proof surface
  already exists.
- No implementation semantics except printer/test-policy cleanup tied to an
  existing named proof surface.

## First Owning Layer

Backend test and diagnostic policy.

## First Consumer Migration

Rewrite one Route 4, Route 5, or Route 7 dump row family to named block-entry
publication agreement, current-block join-source or edge-publication
agreement, or comparison agreement after that named proof surface has landed.

## Proof Surface

Existing runtime, object-runtime, object, MIR, prepared-MIR, or prepared
contract proof first. Named BIR route-view proof is allowed only for semantic
BIR view contracts or temporary compatibility adapters.

## Numbered Route APIs Kept Private Compatibility

- Route 4 transitional dump labels
- `route5_status`
- `route5_agrees`
- `route5_join_source`
- Route 7 route-index status labels
- `RouteIndexReferenceFacade` printer/proof residue

These may remain transitional diagnostic compatibility until their named proof
surface exists. They must not be treated as stable default harness contracts or
as executable authority.

## In Scope

- Rename or gate one route-numbered dump/test vocabulary family after named
  proof exists.
- Preserve or improve proof strength by relying on prepared, MIR, object, or
  runtime checks when behavior is executable.
- Keep temporary route-view tests deletion- or rewrite-pointed.
- Remove default intermediate dump coupling when a stronger connected proof
  already exists.

## Out Of Scope

- Creating named BIR view semantics.
- Migrating prealloc consumers.
- Changing executable prepared, MIR, object, or runtime behavior.
- Weakening expectations, unsupported markers, allowlists, default harness
  contracts, or timeout/accounting policy.

## Acceptance Criteria

- One transitional route-numbered dump/test vocabulary family is retired,
  renamed, or gated behind a named proof surface.
- The proof surface is at least as strong as the old route dump contract for
  the behavior being claimed.
- No unsupported downgrade, allowlist filter, or baseline-only acceptance is
  used as progress.
- Route-numbered labels remain only as private compatibility when still needed.

## Reviewer Reject Signals

- Reject dump text rewrites that lead semantic migration instead of following
  an existing named proof surface.
- Reject expectation rewrites, unsupported-marker changes, allowlist edits,
  timeout/accounting changes, or weaker runtime checks claimed as cleanup.
- Reject route dump proof as sufficient for executable behavior when prepared,
  MIR, object, object-runtime, or runtime proof is available.
- Reject broad test policy churn mixed with BIR view extraction or prealloc
  consumer migration.
- Reject retaining the same route-only assertion under a renamed expected
  output file.

## Closure Notes

Closed after the Route 4 block-entry publication attribution/printer vocabulary
was rewritten behind the named prepared block-entry publication proof surface.
Legacy `route4_*` fields remain only as private compatibility mirrors where
existing prepared-printer and BIR compatibility consumers still need them.

Close-time proof used the focused prepared-fact backend scope:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prealloc_block_entry_publications_test c4cll && ctest --test-dir build -R '^(backend_prealloc_block_entry_publications|backend_cli_dump_prepared_bir_exposes_contract_sections)$' --output-on-failure | tee test_after.log
```

`c4c-regression-guard` passed against the rolled focused baseline at 2/2 tests
passed before and after, with no new failures.
