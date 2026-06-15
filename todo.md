Status: Active
Source Idea Path: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Supported Stale Fixture

# Current Packet

## Just Finished

Step 1 - Locate The Supported Stale Fixture is complete by inspection.
The supported stale fixture is
`check_load_local_dynamic_stack_source_exposes_shared_memory_access` in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, using
`make_load_local_dynamic_stack_source(stale_public, stale_ids, {load_local_source_access(stale_ids), stale_public_load_local_source_access(stale_ids)})`.
The compatible positive fixture is the earlier same function path with
`make_load_local_dynamic_stack_source(prepared, ids, load_local_source_access(ids))`,
which proves `emit_prepared_module(prepared)` and
`consume_edge_publication_move_intent(&lookups, ids.predecessor, ids.successor, 2)`
render exactly `lw a1, 12(s2)`.

The stale public row is
`stale_position_access = find_indexed_prepared_memory_access(&lookups.memory_accesses, stale_ids.successor, 0)`:
same `%src` result value id (`1`) as the selected source, but published at
successor block `join` with stale `address.byte_offset == 16`. The current
authority row is
`current_position_access = find_indexed_prepared_memory_access(&lookups.memory_accesses, stale_ids.predecessor, 0)`,
the predecessor `left` `LoadLocal` row selected by
`publication->source_memory_access`, with pointer base `%base`, offset `12`,
size `4`, align `4`. The public index fact is
`find_indexed_prepared_memory_accesses_by_result_value_id(&lookups.memory_accesses, 1)`
returning `[current_position_access, stale_position_access]`.

The current Route 3 / Route 5 authority row is exercised in
`check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback`: build
`route3_function = make_route5_edge_identity_function(dynamic_ids, true)`,
derive `route3_load` with
`route3_find_memory_access_record(route3_index, 0, Route3MemoryAccessNodeKind::LoadLocal)`,
then derive `route5_memory_edge = route5_cfg_edge_publication_record(...)`.
The agreeing authority is Route 5 status `MemorySource` with
`source_memory_identity_available`, pointing at the same Route 3 `LoadLocal`
instruction, pointer base `%base`, byte offset `12`, size `4`.

## Suggested Next

Start Step 2 - Add The Stale Rejection Proof by combining the supported stale
publication fixture with the real RISC-V backend consumer path:
`consume_edge_publication_move_intent(&lookups, stale_ids.predecessor,
stale_ids.successor, 2, &route5_memory_edge)`. The proof target is that the
stale public `memory_accesses` row at successor `join` / offset `16` is
rejected against the current Route 3 / Route 5 memory-source authority at
predecessor `left` / offset `12`, yielding
`EdgePublicationMoveIntentStatus::UnsupportedSourceHome`,
empty `instruction_text`, `route5_edge_status == MemorySource`,
`!route5_edge_source_agrees`, and `!route3_source_memory_agrees`, while the
compatible positive path still renders exactly `lw a1, 12(s2)`.

## Watchouts

- Reuse the supported stale public `memory_accesses` fixture from idea 275;
  do not use synthetic post-construction mutation as proof evidence.
- Exercise the real RISC-V same-consumer backend path, not helper-only
  formatting or classification.
- Preserve the compatible `LoadLocal` path and exact `lw a1, 12(s2)` output.
- Keep cross-publication, byte-offset coverage, x86 parity, edge-publication
  families, metadata, liveness, aggregate retirement, and draft 155 work out
  of this runbook.
- The current stale fixture already proves helper-only rejection without a
  Route 5 row. Step 2 should avoid stopping there; the acceptance evidence must
  drive the overload of `consume_edge_publication_move_intent` that receives a
  real `Route5CfgEdgePublicationRecord`.
- The real consumer path is
  `consume_edge_publication_move_intent` ->
  `RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent` ->
  `route5_edge_source_agrees_with_prepared_publication` /
  `route3_source_memory_agrees_with_prepared_publication` ->
  `render_edge_publication_source_operand`, which also requires the unique
  public lookup by `source_value_id` to equal `publication->source_memory_access`.

## Proof

Inspection-only packet per supervisor. No build or tests run, and
`test_after.log` was not touched.
