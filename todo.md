Status: Active
Source Idea Path: ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Typed Freshness View Contract

# Current Packet

## Just Finished

Completed Step 2 by adding a typed, reference-only direct edge-publication
current-block join source freshness view on `PreparedMirFunctionView`.

New view surface:

- `PreparedMirDirectEdgePublicationSourceQueryStatus`
- `PreparedMirDirectEdgePublicationSourceStatus`
- `PreparedMirDirectEdgePublicationSourceView`
- `PreparedMirDirectEdgePublicationSourceQuery`
- `PreparedMirFunctionView::current_block_direct_edge_publication_sources(...)`

Accepted rows expose typed source/destination ids, home kinds, destination
register, source register/stack/immediate descriptors where applicable, and
selected `DirectEdgePublicationSource` freshness enum metadata. Rejected rows
keep lowering-authority fields empty and report a fail-closed status instead
of returning raw publication rows.

Extended `backend_prepared_mir_core_comparator` with deterministic in-memory
prepared MIR fixtures covering one accepted named direct-edge publication
source freshness row and one rejected missing-publication row with no exposed
lowering authority.

## Suggested Next

Proceed to Step 3 when delegated: migrate the selected low-risk x86 consumer
program point, `x86::prepared::consume_edge_publication_move_intent(...)`, to
consume this typed view result instead of raw edge-publication lookup authority.

## Watchouts

- Keep Step 2 source/freshness authority on the closed idea 589 producer
  contract. Do not infer it from raw publication shape, move-bundle
  completeness, destination register legality, source-home completeness, Route
  5 text, diagnostics, or prepared printer output.
- Step 2 intentionally did not migrate x86; that remains Step 3 scope.
- Rejected view rows are status-only for lowering purposes. Do not populate
  source/destination operand authority on rejected rows in the x86 migration.
- Do not reopen idea 590 or branch stack-load freshness in this first view
  slice. Branch stack-loads have producer ownership too, but there is no
  comparable x86 consumer surface in the inspected files.
- Do not rewrite shared-prealloc producers or broaden into typed/aggregate
  stack-source publication, select/alias freshness, RV64, AArch64, or broad
  target migration.
- Do not infer source/freshness authority from raw publication rows, stack
  homes, move bundles, route names, debug text, diagnostics, expectations,
  allowlists, timeouts, or baseline policy.

## Proof

Required proof passed and wrote canonical `test_after.log`:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test backend_prepared_mir_core_comparator_test -j && ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure) > test_after.log 2>&1`

Result: 2/2 selected tests passed (`backend_prepared_mir_core_view`,
`backend_prepared_mir_core_comparator`).

Diff hygiene passed:
`git diff --check -- src/backend/mir/prepared_view.hpp src/backend/mir/prepared_view.cpp tests/backend/mir/backend_prepared_mir_core_comparator_test.cpp todo.md`
