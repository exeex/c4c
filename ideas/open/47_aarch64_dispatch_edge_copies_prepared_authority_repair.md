# AArch64 Dispatch Edge Copies Prepared Authority Repair

## Goal

Repair duplicate prepared-authority recovery in
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` by making edge-copy
lowering consume existing shared edge-publication, move-bundle, memory-source,
and source-producer facts, and by adding only the missing shared query needed
for real uncovered edge contexts.

## Why This Exists

The audit found that `dispatch_edge_copies.cpp` already consumes some prepared
edge facts, but still carries fallback producer discovery and late
select-chain materialization paths that can duplicate semantic authority owned
by prealloc/shared prepared records.

## Owned File

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Duplicated Helpers And Fallback Paths

- `prepared_edge_publication_producer_context` should remain a prepared fact
  consumer for `source_producer_block_label`,
  `source_producer_instruction_index`, and `source_producer_kind`; it must not
  be replaced with local source scans.
- `find_edge_named_producer` plus `unique_branch_predecessor_context` rederive
  named producers across successor, edge predecessor, and up to four linear
  predecessors when `prepared_publication == nullptr`.
- `emit_edge_load_local_to_register_impl` must consume prepared source memory
  facts instead of re-scanning BIR load-local/address operands.
- `should_emit_block_entry_edge_copy_move` and
  `lower_predecessor_select_parallel_copy_sources` are suspect when they do
  local predecessor/successor/move matching instead of prepared edge/move
  helpers.
- `emit_select_chain_value_to_register` and
  `materialize_direct_global_select_chain_call_argument` duplicate
  select-chain/direct-global dependency recovery when a prepared call or
  shared select-chain query should own the semantic source.

## Shared Facts To Consume Or Add

- Consume `PreparedEdgePublication`,
  `PreparedEdgePublicationSourceProducer`,
  `PreparedEdgePublicationSourceProducerLookups`,
  `PreparedFunctionLookups::edge_publication_source_producers`,
  `find_indexed_prepared_edge_publication_source_producer`, and
  `find_unique_indexed_prepared_edge_publication`.
- Consume `PreparedEdgePublication::source_memory_*`,
  `PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedMemoryAccess`,
  and `PreparedValueHome` for load-local source materialization.
- Consume `PreparedMoveBundle`,
  `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`,
  `find_prepared_move_bundle`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`,
  `prepared_edge_publication_redundant_block_entry_parallel_copy_move`, and
  `prepared_edge_publication_matches_parallel_copy_move_source`.
- Add a prepared edge dependency query by source value plus edge context only
  if callers that pass `prepared_publication == nullptr` prove unsupported by
  current lookups.
- Decide whether late select-chain call-argument materialization belongs to
  `PreparedCallPlan` or a shared select-chain dependency query before touching
  the fallback path.

## Out Of Scope

- Do not change AArch64 register-read hazard checks, scratch ordering, final
  move/load/store/select instruction spelling, or target-local register
  selection unless the change is strictly needed to consume shared authority.
- Do not combine this repair with other audited AArch64 files.
- Do not treat line-count reduction as acceptance.

## Acceptance Criteria

- `find_edge_named_producer` and predecessor-depth fallback use is either
  removed, fail-closed behind prepared facts, or justified by a documented
  shared-query gap with a corresponding shared query added.
- Edge load-local materialization consumes prepared source memory facts and
  value homes without raw BIR address/source rediscovery.
- Block-entry edge-copy redundancy and parallel-copy source matching consume
  prepared edge-publication and move-bundle helpers.
- Select-chain/direct-global late materialization no longer grows local
  named-case logic and is routed through the chosen shared authority.
- Narrow proof covers at least one edge-publication/source-producer case and
  any remaining `prepared_publication == nullptr` fallback route.

## Reviewer Reject Signals

- Reject adding a fresh same-block scan, predecessor-depth scan, or
  name-shaped producer match in place of prepared source-producer fields.
- Reject extending `find_edge_named_producer` or
  `unique_branch_predecessor_context` depth as the repair.
- Reject bypassing `PreparedEdgePublication` or `PreparedMoveBundle` helpers
  for predecessor/successor/move matching.
- Reject direct-global/select-chain special cases that only make a named
  call-argument case pass.
- Reject expectation downgrades, unsupported-test rewrites, or claims of
  capability progress based only on helper renames or comment changes.
