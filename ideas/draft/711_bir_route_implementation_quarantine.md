# BIR Route Implementation Quarantine

Status: Open
Type: BIR internal boundary retirement
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After:
- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`
- `ideas/open/706_common_mir_named_query_migration.md`
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `ideas/open/710_rv64_named_handoff_materializer_cleanup.md`

## First Owner And Scope

First owning layer: BIR internals.  Quarantine, rename, combine, or delete
`bir_route*.cpp`, the facade, route-index headers, and prerequisites only after
all semantic downstream consumers use named contracts.

First migrated consumer: the private adapters/builders behind the named BIR
views from idea 704.

## Dependencies And Proof

- Depends on prepared, common MIR, and all target semantic migrations.
- Proof surface: BIR named-contract tests, public-header/include dependency
  checks, and the full route-vocabulary guard classified by private residue.

## Retirement Guard

No prealloc, MIR, target, or public test file may mention route vocabulary.
Any remaining hit must be private BIR implementation with a named-view caller
and a documented deletion condition; new route families are forbidden.

## Acceptance Criteria

- Route headers are private or removed and cannot cross owner boundaries.
- Facade/status/prerequisite surfaces shrink; private algorithms remain only
  where named semantic producer proof needs them.
- Every retained file has an explicit retirement condition and no public test
  fixture dependency.

## Reviewer Reject Signals

- File renames are claimed as architectural retirement.
- A compatibility adapter remains publicly includable or grows new callers.
- Builders are deleted before equivalent named semantic behavior is proved.
