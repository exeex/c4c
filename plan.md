# AArch64 Dispatch Edge Copies Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md

## Purpose

Turn `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` into a
consumer of shared prepared edge-publication, move-bundle, memory-source, and
source-producer facts instead of letting it recover the same semantic authority
through local scans and late named-case materialization.

## Goal

Remove or fail-close duplicate prepared-authority recovery in edge-copy
lowering while preserving emitted AArch64 behavior.

## Core Rule

Prefer prepared/shared authority over local rediscovery. Do not add new
same-block scans, predecessor-depth scans, name-shaped producer matching, or
expectation downgrades as progress.

## Read First

- `ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- Prepared authority definitions and lookup helpers for:
  - `PreparedEdgePublication`
  - `PreparedEdgePublicationSourceProducer`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedFunctionLookups::edge_publication_source_producers`
  - `PreparedEdgePublication::source_memory_*`
  - `PreparedMemoryAccess`
  - `PreparedValueHome`
  - `PreparedMoveBundle`
  - `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`

## Current Targets

- `prepared_edge_publication_producer_context`
- `find_edge_named_producer`
- `unique_branch_predecessor_context`
- `emit_edge_load_local_to_register_impl`
- `should_emit_block_entry_edge_copy_move`
- `lower_predecessor_select_parallel_copy_sources`
- `emit_select_chain_value_to_register`
- `materialize_direct_global_select_chain_call_argument`

## Non-Goals

- Do not change AArch64 register-read hazard checks, scratch ordering,
  instruction spelling, or target-local register selection unless required to
  consume prepared authority.
- Do not combine this repair with other audited AArch64 files.
- Do not claim line-count reduction, helper renames, or expectation rewrites as
  capability progress.
- Do not downgrade supported-path tests to `unsupported`.

## Working Model

- `PreparedEdgePublication` and its indexed source-producer records own edge
  producer identity.
- Prepared source memory facts and value homes own load-local source
  materialization.
- `PreparedMoveBundle` and prepared edge-publication helpers own
  block-entry/out-of-SSA parallel-copy matching.
- Late select-chain/direct-global materialization must be routed through an
  existing prepared call/select-chain authority or through a new shared query,
  not through local named-case logic.

## Execution Rules

- Keep each code-changing step behavior-preserving unless the source idea
  explicitly requires fail-closed behavior for unsupported local rediscovery.
- If a caller still passes `prepared_publication == nullptr`, prove whether the
  current shared lookups can cover that edge context before adding a new query.
- Add a shared prepared dependency query only for a real uncovered edge context
  and make its ownership explicit in the proof.
- Each implementation packet needs fresh build or compile proof. The supervisor
  chooses the exact proving subset.
- Narrow proof must cover at least one edge-publication/source-producer case
  and any remaining `prepared_publication == nullptr` fallback route.

## Steps

### Step 1: Map existing prepared and fallback paths

Goal: establish where `dispatch_edge_copies.cpp` already has prepared authority
and where it still performs local recovery.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

Actions:

- Inspect every `prepared_publication` caller and record which contexts can be
  routed to `PreparedEdgePublication` or indexed source-producer lookups.
- Inspect `find_edge_named_producer` and `unique_branch_predecessor_context`
  call sites and identify whether each use has a prepared-publication path.
- Inspect `emit_edge_load_local_to_register_impl` for raw BIR load-local or
  address rediscovery that duplicates prepared source memory facts.
- Inspect block-entry and select-chain helpers for local predecessor,
  successor, move, or direct-global matching.

Completion check:

- `todo.md` names the exact first fallback route to repair and the proof subset
  the supervisor selected for that route.

### Step 2: Replace source-producer fallback recovery

Goal: remove, fail-close, or justify `find_edge_named_producer` and
predecessor-depth rediscovery behind prepared source-producer facts.

Primary target: `find_edge_named_producer`,
`unique_branch_predecessor_context`, and call sites that pass
`prepared_publication == nullptr`

Actions:

- Preserve `prepared_edge_publication_producer_context` as a consumer of
  `source_producer_block_label`, `source_producer_instruction_index`, and
  `source_producer_kind`.
- Prefer `find_indexed_prepared_edge_publication_source_producer` and
  `find_unique_indexed_prepared_edge_publication` before local producer
  recovery.
- For uncovered null-publication callers, either route them to existing
  prepared lookups or add the minimal shared query by source value plus edge
  context.
- Do not expand predecessor-depth matching or introduce new name-shaped scans.

Completion check:

- Source-producer lowering no longer depends on a local predecessor-depth scan
  except for a documented fail-closed/shared-query gap, and narrow proof covers
  the repaired source-producer route.

### Step 3: Route load-local source materialization through prepared memory facts

Goal: make edge load-local materialization consume prepared source memory facts
and value homes.

Primary target: `emit_edge_load_local_to_register_impl`

Actions:

- Replace raw BIR load-local/address operand rediscovery with
  `PreparedEdgePublication::source_memory_*`,
  `PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedMemoryAccess`,
  and `PreparedValueHome`.
- Keep final load/store/register behavior stable unless prepared authority
  proves the previous path was unsupported.
- Fail closed when required prepared source memory authority is absent instead
  of reconstructing it locally.

Completion check:

- Load-local edge-copy materialization is driven by prepared memory/value-home
  facts and has compile or narrow test proof for the affected route.

### Step 4: Use prepared move-bundle authority for block-entry edge copies

Goal: ensure block-entry edge-copy redundancy and parallel-copy source matching
consume prepared edge-publication and move-bundle helpers.

Primary target: `should_emit_block_entry_edge_copy_move` and
`lower_predecessor_select_parallel_copy_sources`

Actions:

- Consume `PreparedMoveBundle` and
  `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`.
- Prefer `find_prepared_move_bundle`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`,
  `prepared_edge_publication_redundant_block_entry_parallel_copy_move`, and
  `prepared_edge_publication_matches_parallel_copy_move_source`.
- Remove or fail-close local predecessor/successor/move matching that
  duplicates those helpers.

Completion check:

- Block-entry edge-copy decisions depend on prepared edge/move helpers, and
  proof covers a block-entry or out-of-SSA parallel-copy edge-copy route.

### Step 5: Route select-chain/direct-global late materialization through shared authority

Goal: stop growing local direct-global/select-chain dependency recovery in
edge-copy lowering.

Primary target: `emit_select_chain_value_to_register` and
`materialize_direct_global_select_chain_call_argument`

Actions:

- Decide whether the route belongs to `PreparedCallPlan` or a shared
  select-chain dependency query before editing fallback behavior.
- Replace named-case/direct-global local recovery with the chosen prepared or
  shared authority.
- Avoid changes whose only evidence is one call-argument testcase passing.

Completion check:

- Select-chain/direct-global late materialization uses the chosen shared
  authority and proof covers the repaired edge or call-argument route plus a
  nearby same-feature case where available.

### Step 6: Consolidate proof and regression guard expectations

Goal: ensure the completed repair is not a testcase-overfit slice.

Primary target: lifecycle proof in `todo.md`, `test_before.log`, and
`test_after.log`

Actions:

- Confirm all remaining local fallback paths are either removed, fail-closed,
  or documented as shared-query gaps with a corresponding shared query.
- Confirm no expectations were downgraded and no unsupported-path rewrites were
  used as capability proof.
- Run the supervisor-selected broader validation once multiple narrow packets
  have landed or the repair is ready to close.

Completion check:

- `todo.md` records fresh proof covering edge-publication/source-producer,
  remaining null-publication fallback, load-local source materialization,
  block-entry/move-bundle matching, and select-chain/direct-global authority as
  applicable.
