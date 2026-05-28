Status: Active
Source Idea Path: ideas/open/62_aarch64_shared_edge_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Edge Publication Fallbacks

# Current Packet

## Just Finished

Step 1 - Inventory Edge Publication Fallbacks completed without
implementation edits.

Inventory:

- `find_edge_named_producer` currently has no direct callers in
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`; AST caller query
  returned `target not called in this translation unit`, and raw search only
  found the definition. Its local recovery algorithm still matters as dead or
  stale fallback authority: it tries the successor block before the requested
  instruction index, then the edge block, then up to four unique branch
  predecessors through `unique_branch_predecessor_context`, using
  `mir::find_same_block_named_producer` plus `producer_instruction_index`.
- Live null-publication recovery paths are the `PreparedEdgePublication* ==
  nullptr` branches in edge publication emission. Both
  `emit_edge_value_publication_to_register_impl` and
  `edge_value_publication_may_read_register_index` use
  `prepared_edge_named_source_producer_context` when no prepared publication is
  supplied; that resolves a named value through
  `context.function.prepared_lookups->edge_publication_source_producers`, then
  reopens the producer block/instruction through
  `prepared_edge_source_producer_context`.
- `emit_edge_load_local_to_register_impl` has the same split for source memory:
  with a matching prepared publication it requires
  `prepared_publication_source_memory_access`; without one it falls back to
  `prepared_memory_access(producer.context, producer.instruction_index)`, then
  to source register/home materialization. Prepared-publication load-local
  failure is fail-closed; null-publication load-local failure can still recover
  from local prepared memory/home facts.
- `lower_predecessor_join_source_publication` and
  `should_emit_block_entry_edge_copy_move` consume
  `PreparedEdgePublication` and call
  `prepared_edge_publication_producer_context`; these are prepared-publication
  paths, not null-publication recovery, but they still depend on publication
  source producer block/instruction facts being complete.
- `lower_predecessor_select_parallel_copy_sources` is the target block-entry
  edge-copy route. It gets the predecessor/successor edge from the prepared
  move bundle, finds the unique block-entry parallel-copy edge publication with
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`, verifies
  `prepared_edge_publication_matches_parallel_copy_move_source`, and delegates
  source emission to `lower_predecessor_join_source_publication`.
- `materialize_direct_global_select_chain_call_argument` is not part of the
  edge-copy publication route, but it has a separate fallback source-producer
  construction path: if `context.function.prepared_lookups` is absent and
  prepared/control-flow state exists, it builds local
  `make_prepared_edge_publication_source_producer_lookups`. This is a
  target-local select-chain fallback and should stay out of the edge-copy
  migration unless the supervisor explicitly widens scope.

Source facts needed:

- Predecessor and successor labels for edge-copy identity come from
  `PreparedEdgePublication::{predecessor_label, successor_label}` and from
  `PreparedMoveResolution::source_parallel_copy_{predecessor,successor}_label`.
- Destination facts come from the edge publication key
  `(predecessor_label, successor_label, destination_value_id)` and the
  publication's `destination_value`, `destination_value_name`,
  `destination_home`, and destination storage kind.
- Source identity requires `source_value`, `source_value_name`,
  `source_value_id`, `source_home`, `source_home_kind`,
  `source_producer_kind`, `source_producer_block_label`, and
  `source_producer_instruction_index`. Load-local sources additionally require
  `source_memory_access_status`, `source_memory_access`, and copied address
  fields.
- Target-only responsibilities that should remain AArch64-local are scalar
  register spelling, scratch selection, load/mov/cast/binary/select-chain
  instruction emission, va-list carrier loads, register-read hazard checks,
  clobber filtering in `should_emit_block_entry_edge_copy_move`, and emitted
  scalar-register bookkeeping.

Shared owner candidates:

- `PreparedEdgePublication` in
  `src/backend/prealloc/prepared_lookups.hpp` already carries most
  predecessor/successor/edge/source publication facts.
- `make_prepared_edge_publication_lookups` in
  `src/backend/prealloc/prepared_lookups.cpp` already builds publications from
  `PreparedJoinTransfer::edge_transfers`, applies source producer facts with
  `apply_source_producer_fact`, copies load-local memory facts with
  `apply_source_memory_access_fact`, records parallel-copy/move authority, and
  indexes by edge destination.
- `PreparedEdgePublicationSourceProducerLookups` and
  `find_indexed_prepared_edge_publication_source_producer` are the existing
  shared producer index candidates. They currently index producers globally by
  value name; duplicate publications are collapsed to `Unknown`, which is a
  useful fail-closed signal but not yet a structured edge-scoped diagnostic.
- Existing publication-query helpers
  `find_unique_indexed_block_entry_parallel_copy_edge_publication`,
  `prepared_edge_publication_matches_parallel_copy_move_source`, and
  `prepared_edge_publication_redundant_block_entry_parallel_copy_move` already
  answer parts of the edge-copy question for block-entry parallel-copy moves.

Missing shared fact gaps:

- There is no single prepared query that returns a complete edge-copy source
  fact bundle for `(predecessor, successor, move/destination)` and validates
  source value, source home, source producer, memory access status, and
  fail-closed reason together.
- The null-publication emitters can still query named producers without an
  edge-scoped publication, so they can emit from global prepared producer facts
  instead of an authoritative predecessor/successor publication.
- Missing or ambiguous producer facts collapse to `Unknown` or `nullptr`, but
  the edge-copy path lacks a durable reason distinguishing duplicate producer,
  missing prepared lookup, missing source home, missing memory access,
  incomplete memory access, stale producer pointer, and wrong edge.
- `find_edge_named_producer` is dead today but should be deleted or explicitly
  fail-closed rather than retained as a predecessor-depth fallback authority
  when Step 4 removes recovery.

Negative/fail-closed cases to preserve or add:

- Empty or non-named values, invalid block labels, missing prepared lookups,
  missing producer block/instruction, stale producer pointer/type mismatch,
  missing source home, non-register destination for register edge copy, missing
  load-local memory access, incomplete memory access, ambiguous duplicate
  source producer, publication/move edge mismatch, missing unique publication,
  immediate/unknown producer where materialized producer emission is required,
  select-chain recursion/hazard failure, and scratch exhaustion should all
  return no edge-copy emission or diagnose explicitly rather than falling back
  to predecessor-depth source discovery.

## Suggested Next

Step 2 - Define Shared Edge Source Facts: add or extend a shared prepared edge
query that returns a complete edge-copy source fact bundle for a
predecessor/successor/destination move and reports deliberate missing-fact
statuses.

## Watchouts

`find_edge_named_producer` has no caller, so the live risk is not that exact
function being invoked; it is the null-publication branches in
`emit_edge_value_publication_to_register_impl`,
`edge_value_publication_may_read_register_index`, and
`emit_edge_load_local_to_register_impl` still using named producer/memory/home
recovery without an edge-scoped publication. Keep non-edge select-chain
fallbacks such as `materialize_direct_global_select_chain_call_argument` out of
scope unless the supervisor widens the packet.

## Proof

`printf 'Inventory-only Step 1; no build or test proof required because no implementation files changed.\n' > test_after.log`

Proof was sufficient for this inventory-only packet because no implementation
files changed. Log path: `test_after.log`.
