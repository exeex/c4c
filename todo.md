Status: Active
Source Idea Path: ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Low-Risk x86 Consumer Or Comparator Path

# Current Packet

## Just Finished

Completed Step 3 by migrating the selected x86 direct edge-publication move
consumer, `x86::prepared::consume_edge_publication_move_intent(...)`, to use
`PreparedMirFunctionView::current_block_direct_edge_publication_sources(...)`
as the source/freshness lowering authority.

The module compare-join lowering path now passes the existing
`PreparedMirFunctionView` and join block index into the consumer. The consumer
only renders source/destination operands from an accepted typed
`PreparedMirDirectEdgePublicationSourceView` row matching the requested
predecessor, successor, and destination value. Rejected or missing typed rows
fail closed without exposing operands.

The legacy raw `PreparedEdgePublication` pointer remains only as compatibility
metadata for the existing Route 5 agreement/type checks after typed admission;
it is no longer used to derive source operands, destination operands, or
freshness authority in this selected path.

## Suggested Next

Proceed to Step 4 when delegated: audit the freshness-view boundary and close
readiness for this first direct edge-publication source/freshness contract.
Record any remaining consumer migrations or source/freshness families as
follow-up scope rather than expanding this active plan.

## Watchouts

- Rejected view rows remain status-only for lowering purposes. The x86
  migrated path must continue to avoid sourcing operands from rejected rows or
  raw publication shape.
- Raw `PreparedEdgePublication` is still present in the intent only for the
  existing Route 5 compatibility/type checks. Do not treat that pointer as
  permission to reintroduce raw source/freshness authority.
- Do not reopen idea 590 or branch stack-load freshness in this first view
  family. Branch stack-loads have producer ownership too, but they are not the
  selected consumer for this packet.
- Do not rewrite shared-prealloc producers or broaden into typed/aggregate
  stack-source publication, select/alias freshness, RV64, AArch64, or broad
  target migration.
- Do not infer source/freshness authority from raw publication rows, stack
  homes, move bundles, route names, debug text, diagnostics, expectations,
  allowlists, timeouts, or baseline policy.

## Proof

Required proof passed and wrote canonical `test_after.log`:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test backend_prepared_mir_core_comparator_test -j && ctest --test-dir build -R '^backend_prepared_mir_|^backend_x86_shared_producer_query$|^backend_codegen_route_x86_64_.*observe_semantic_bir$' --output-on-failure) > test_after.log 2>&1`

Result: 79/79 selected tests passed.

Diff hygiene passed:
`git diff --check -- src/backend/mir/x86/prepared/prepared.hpp src/backend/mir/x86/prepared/dispatch.cpp src/backend/mir/x86/module/module.cpp todo.md`
