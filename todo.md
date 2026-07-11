Status: Active
Source Idea Path: ideas/open/692_prepared_mir_source_dependency_freshness_view_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Source/Freshness Authority Family

# Current Packet

## Just Finished

Completed Step 1 evidence selection for the first source/freshness view
contract.

Selected family: direct edge-publication current-block join parallel-copy
source freshness, from closed idea 589.

Why this family fits:

- Idea 589 closed with an explicit ownership rule: direct edge-publication
  moves require selected `DirectEdgePublicationSource` freshness for the exact
  source being accepted. Destination bundle legality, source-home completeness,
  route/debug text, and local move shape are not source authority.
- The producer path already exists in shared prealloc through
  `prepare_block_entry_parallel_copy_edge_source_facts(...)` and
  `prepare_current_block_join_parallel_copy_source_facts(...)`, with selected
  freshness carried on `PreparedCurrentBlockJoinParallelCopySourceFact`.
- Current `PreparedMirFunctionView` already owns view-scoped
  `PreparedFunctionLookups`, including edge-publication lookups and lookup
  counts, so Step 2 can layer a reference-only feature view without rewriting
  producers.
- x86 has a real low-risk consumer program point:
  `src/backend/mir/x86/prepared/dispatch.cpp::consume_edge_publication_move_intent`
  and its caller in
  `src/backend/mir/x86/module/module.cpp::append_prepared_compare_join_parallel_copy`
  currently read raw edge-publication lookup rows for compare-join edge moves.

Accepted lowering inputs that Step 2 may expose:

- An admitted current-block join direct edge-publication source result only
  when the shared producer facts report available source authority for the
  exact edge/source/destination.
- Typed identity for function, predecessor label, successor label,
  destination value id/name, selected source value id/name, source/destination
  home kind, destination storage/register, and immediate-source state.
- Selected freshness authority metadata only as typed enums/ids:
  `DirectEdgePublicationSource` use,
  `DirectEdgePublication` source kind/proof/rank, selected source value,
  candidate count/status, and the exact edge/publication/move reference needed
  to prove the producer-backed source.
- View-owned read-only references may point at prepared facts, but lowering
  consumers should receive the admitted typed result rather than a raw
  publication-row escape hatch.

Rejected/fail-closed admission states:

- Missing `PreparedMirFunctionView`, missing shared lookups, missing
  value-location/regalloc/control-flow context, missing edge-publication
  lookups, missing block/successor label, missing publication, missing move,
  missing selected freshness authority, no-candidate freshness, ambiguous
  freshness, invalid freshness, unsupported freshness, wrong source value,
  wrong use kind, stale edge/program point, destination-only authority,
  structurally complete source home without freshness, and Route 5/debug text
  agreement without producer freshness.
- Immediate sources may remain authority-free only where the existing producer
  contract intentionally treats them that way; Step 2 must not generalize that
  exception to named direct edge-publication sources.

Step 2 consumer/test surface:

- Primary structural test surface: add focused MIR view tests that construct or
  reuse a prepared compare-join/current-block-join fixture and assert accepted
  direct edge-publication source facts become typed view results while missing
  or invalid freshness remains fail-closed and unconsumable.
- Candidate follow-up consumer program point for Step 3:
  `x86::prepared::consume_edge_publication_move_intent(...)`, replacing raw
  edge-publication source authority with the admitted view result.

## Suggested Next

Proceed to Step 2 when delegated: add the typed direct edge-publication source
freshness feature view under `src/backend/mir/prepared_view.*` plus focused
MIR tests for one accepted fact and at least one fail-closed rejected fact.

## Watchouts

- Keep Step 2 source/freshness authority on the closed idea 589 producer
  contract. Do not infer it from raw publication shape, move-bundle
  completeness, destination register legality, source-home completeness, Route
  5 text, diagnostics, or prepared printer output.
- Do not migrate x86 in Step 2 unless the supervisor explicitly widens the
  packet; the selected x86 program point is identified for Step 3.
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

- Evidence-only packet. No build or CTest required.
- Inspection commands used included `sed` over ideas 589/590,
  `src/backend/mir/prepared_view.*`, `plan.md`, and the active idea, plus `rg`
  over `src/backend/mir/x86/`, `src/backend/prealloc/`, and focused backend
  tests for edge-publication, current-block-join, branch-stack-load, and
  freshness surfaces.
- Proposed Step 2 proof command:
  `set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test backend_prepared_mir_core_comparator_test -j && ctest --test-dir build -R '^backend_prepared_mir_' --output-on-failure) > test_after.log 2>&1`
