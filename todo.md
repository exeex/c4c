# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map existing prepared and fallback paths

## Just Finished

Step 1 mapping is complete for
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.

Prepared-publication caller/context map:

- `prepared_edge_publication_producer_context` consumes
  `PreparedEdgePublication::source_producer_block_label`,
  `source_producer_instruction_index`, `source_producer_kind`, and the cached
  `source_load_local`/`source_cast`/`source_binary`/`source_select` pointers.
  AST callers are `edge_value_publication_may_read_register_index`,
  `emit_edge_value_publication_to_register_impl`, and
  `lower_predecessor_join_source_publication`.
- `lower_predecessor_join_source_publication` is the prepared root route. It is
  reached from `lower_predecessor_select_parallel_copy_sources` after a
  prepared block-entry publication is found, rejects Unknown/Immediate
  producer kinds, uses `prepared_edge_publication_producer_context`, then calls
  `emit_edge_value_publication_to_register(..., &publication)`.
- `emit_edge_value_publication_to_register` rejects a non-null publication that
  does not match the current root value, then calls the impl with that
  publication. Repo-local search found no callers outside this file.
- `emit_edge_value_publication_to_register_impl` uses
  `prepared_edge_publication_producer_context` only when the non-null
  publication matches the current value. Root `LoadLocal`, `Cast`, `Binary`,
  and `SelectMaterialization` producers can use prepared producer context.
  Recursive child operands inside those producers often no longer match the
  root publication, so they fall back to `find_edge_named_producer`.
- `edge_value_publication_may_read_register_index` has the same split: a
  matching non-null publication uses prepared producer context, while null or
  child-value mismatch uses `find_edge_named_producer`; recursive cast/binary/
  select hazard checks carry the same publication but only the root matches.
- `emit_edge_load_local_to_register` has no repo-local call sites outside this
  file; the impl is reached from the edge value materializer for LoadLocal
  producers and receives the publication only when the root LoadLocal result
  matches it.

Fallback producer call sites:

- `find_edge_named_producer` is called only by
  `edge_value_publication_may_read_register_index` and
  `emit_edge_value_publication_to_register_impl`.
- `find_edge_named_producer` scans `successor_context` before
  `successor_before_instruction_index`, then the edge block to the end, then up
  to four unique branch predecessors.
- `unique_branch_predecessor_context` is called only inside
  `find_edge_named_producer` and locally rederives a unique predecessor by
  scanning prepared control-flow blocks for a branch to the current block.
- Existing shared facts that can cover this route are
  `PreparedFunctionLookups::edge_publication_source_producers` and
  `find_indexed_prepared_edge_publication_source_producer`, which already index
  LoadLocal/Cast/Binary/SelectMaterialization producers by prepared value name.

Load-local source-memory behavior:

- `emit_edge_load_local_to_register_impl` does not consume
  `PreparedEdgePublication::source_memory_*` fields or
  `source_memory_access_status`. It re-queries `prepared_memory_access` by
  producer block/instruction index from prepared addressing.
- If no prepared memory access is found, it falls back to generic
  `emit_value_publication_to_register(producer.context, load.result, ...)`,
  which is raw publication recovery from the producer context rather than
  publication-owned source-memory authority.
- Frame-slot accesses emit a direct stack load from the looked-up
  `PreparedMemoryAccess`. Pointer-value accesses materialize the pointer value
  with `emit_edge_value_publication_to_register_impl`; when that pointer value
  does not match the root publication, producer discovery can again fall back
  to `find_edge_named_producer`.
- `prepared_memory_access_matches_instruction` exists but is not used in this
  path.

Block-entry and move-bundle helper behavior:

- `should_emit_block_entry_edge_copy_move` already consumes
  `find_unique_indexed_block_entry_parallel_copy_edge_publication` and
  `prepared_edge_publication_redundant_block_entry_parallel_copy_move` for
  OutOfSsa block-entry moves with destination registers and prepared lookups.
  It still has local fallback filters: current-join clobber suppression,
  source/destination register alias suppression, and source-memory suppression.
- `lower_predecessor_select_parallel_copy_sources` already consumes
  `find_prepared_move_bundle`,
  `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`,
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`, and
  `prepared_edge_publication_matches_parallel_copy_move_source`. It still does
  local prefilters for branch shape, successor spelling, source/destination
  homes, and stack/register source shape before calling
  `lower_predecessor_join_source_publication`.

Select-chain/direct-global fallback behavior:

- `emit_select_chain_value_to_register` is a separate local select-chain
  materializer. It uses `find_same_block_select_producer` and generic
  `emit_value_publication_to_register`; it has no prepared publication,
  prepared call-plan, or shared select-chain dependency authority.
- `materialize_direct_global_select_chain_call_argument` runs before prepared
  call argument plan lookup in `lower_scalar_call_argument_producers`. It gates
  on `select_chain_contains_direct_global_load`, local value-home checks, then
  emits via `emit_fp_value_to_register` or `emit_select_chain_value_to_register`.
  This is the direct-global/select-chain fallback route to handle after the
  source-producer and source-memory repairs.

Exact first fallback route to repair:

Repair `emit_edge_value_publication_to_register_impl`/`edge_value_publication_may_read_register_index`
where child operands of a prepared root publication and all null-publication
contexts call `find_edge_named_producer`. The first implementation slice should
add or use a helper that resolves a named value through
`PreparedFunctionLookups::edge_publication_source_producers` and constructs the
same `EdgeProducerContext` from the indexed producer facts, then fail closed
instead of scanning successor/edge/predecessor blocks when prepared lookup
authority is available.

## Suggested Next

Implement Step 2 for the first fallback route: replace the
`find_edge_named_producer` path in the edge value materializer and register
hazard helper with prepared source-producer lookup by value name plus edge
context, without expanding local scans or predecessor-depth matching.

## Watchouts

- The current root prepared-publication tests already exercise prepared
  Binary/Select roots, but nested operands can still silently use local
  rediscovery. The next slice should prove child operand producer lookup, not
  just root producer lookup.
- Do not broaden `unique_branch_predecessor_context`, add same-block scans, or
  make a named-case shortcut. Existing `find_edge_named_producer` behavior is
  the authority leak being repaired.
- Source-memory repair is a separate Step 3 slice: do not mix
  `PreparedEdgePublication::source_memory_*` consumption into the first
  source-producer fallback patch unless it is strictly required to compile.
- Select-chain/direct-global call argument materialization is later Step 5
  work and should be routed through prepared call/select-chain authority, not
  extended locally.

## Proof

Not run; no build/test proof was required for this mapping-only packet, and no
`test_after.log` was produced.

Supervisor-proof subset recommendation for the first repair route:

`cmake --build build --target backend_aarch64_instruction_dispatch backend_prepared_lookup_helper && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' --output-on-failure`

This subset covers the AArch64 edge-publication/source-producer materialization
cases in `backend_aarch64_instruction_dispatch` and the prepared indexed
source-producer lookup facts in `backend_prepared_lookup_helper`.
