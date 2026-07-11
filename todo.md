Status: Active
Source Idea Path: ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit Boundaries And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 by auditing the typed direct edge-publication
source/freshness boundary and recording close-readiness evidence for the
active plan.

Boundary evidence:

- `PreparedMirFunctionView::current_block_direct_edge_publication_sources(...)`
  is the MIR-facing authority surface for this family. It calls the existing
  current-block join parallel-copy source producer and maps producer statuses
  into typed query/source statuses.
- Accepted `PreparedMirDirectEdgePublicationSourceView` rows are populated only
  after the producer reports an available fact, the destination home is a
  register, and the selected freshness authority has the expected direct
  edge-publication use/source/proof/rank enum values.
- Rejected rows are pushed before source/destination lowering fields are
  populated. Missing function view, block, lookups, value locations,
  edge-publication lookups, or successor label return fail-closed query
  statuses.
- The selected x86 consumer,
  `x86::prepared::consume_edge_publication_move_intent(...)`, queries
  `current_block_direct_edge_publication_sources(successor_block_index)`,
  selects only an accepted typed row matching predecessor, successor, and
  destination value id, and derives source operand, destination operand, and
  instruction text from that typed row.
- If no accepted matching typed row is present, the x86 consumer returns
  `MissingPublication`; unsupported source/destination typed fields return
  unsupported statuses and do not emit.
- The remaining raw `PreparedEdgePublication` pointer in the migrated path is
  obtained only after typed admission and is used by the existing Route 5
  agreement/type compatibility checks, not for source operands, destination
  operands, or freshness authority.
- Recent Step 2/3 changed files were limited to `todo.md`, the prepared MIR
  view implementation/header, the focused prepared MIR comparator test, and
  the selected x86 module/prepared files. No expectations, unsupported markers,
  allowlists, timeout/baseline policy files, `plan.md`, `ideas/open`, broad
  producer rewrites, non-x86 targets, or unrelated source/freshness families
  were changed.

## Suggested Next

Ready for supervisor validation and plan-owner closure review of the active
source idea.

Follow-up scope outside this source idea: migrate any additional x86 consumers
that still intentionally use broader prepared/prealloc authority, add broader
raw-dependency gates if desired, and address other freshness families such as
branch stack-load freshness, typed/aggregate stack-source publication,
select/alias freshness, and non-x86 targets under separate source ideas.

## Watchouts

- This plan deliberately covers one selected family and one selected x86
  consumer migration. Additional migrations should be new scoped work, not a
  closure prerequisite for this plan.
- Keep the raw publication pointer in this migrated path constrained to
  compatibility/type/Route 5 agreement checks after typed admission.
- Rejected typed rows must remain status-only for lowering authority.

## Proof

Step 4 was audit-only; no build rerun was required because the audit found no
implementation correction.

Previously required Step 3 proof remains available in canonical
`test_after.log` and passed:
`set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test backend_prepared_mir_core_comparator_test -j && ctest --test-dir build -R '^backend_prepared_mir_|^backend_x86_shared_producer_query$|^backend_codegen_route_x86_64_.*observe_semantic_bir$' --output-on-failure) > test_after.log 2>&1`

Result: 79/79 selected tests passed.

Audit commands run:

- `git log --oneline -5 --decorate`
- `git show --stat --oneline --name-only HEAD`
- `git show --name-only --pretty=format: HEAD~3..HEAD`
- `git diff --name-only HEAD~2..HEAD`
- `rg -n "current_block_direct_edge_publication_sources|PreparedMirDirectEdgePublicationSource(View|Status|QueryStatus)|consume_edge_publication_move_intent|PreparedEdgePublication\\* publication|source_operand|destination_operand|source_freshness" src/backend/mir/prepared_view.hpp src/backend/mir/prepared_view.cpp src/backend/mir/x86/prepared/dispatch.cpp src/backend/mir/x86/prepared/prepared.hpp src/backend/mir/x86/module/module.cpp`
- `rg -n "find_unique_indexed_prepared_edge_publication|source_home|destination_home|source_freshness_authority|source_operand =|destination_operand =" src/backend/mir/x86/prepared/dispatch.cpp src/backend/mir/x86/module/module.cpp`
- `grep -E "100% tests passed|tests passed, 0 tests failed out of 79|backend_prepared_mir_|backend_x86_shared_producer_query" -n test_after.log`

Audit result: close-ready for supervisor validation and plan-owner closure
review. No blockers found.
