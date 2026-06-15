Status: Active
Source Idea Path: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Same-Consumer Prepared Row Facts

# Current Packet

## Just Finished

Completed Step 1 fact capture for the RISC-V dynamic stack-source `LoadLocal`
row before any code or test edits.

Same-consumer backend path:

- The real RISC-V consumer is
  `riscv::emit_prepared_module(const PreparedBirModule&)`, which builds
  per-function normal prepared lookups with
  `prepare::make_prepared_function_lookups(...)`, iterates
  `lookups.edge_publications.publications`, and calls
  `append_edge_publication_move_instruction(...)`.
- The direct consumer of `PreparedFunctionLookups::memory_accesses` is
  `render_edge_publication_source_operand(...)` in
  `src/backend/mir/riscv/codegen/emit.cpp`. In the dynamic stack-source branch
  it calls
  `prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(&lookups->memory_accesses, *publication.source_value_id)`
  and fails the source operand if the indexed row is absent or is not pointer
  identical to `publication.source_memory_access`.
- The externally testable helper for that same consumer is
  `riscv::consume_edge_publication_move_intent(...)`; it routes through
  `RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent()` and
  the same `render_edge_publication_source_operand(...)` function.

Concrete idea 271 prepared memory row:

- Fixture:
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
  `check_load_local_dynamic_stack_source_exposes_shared_memory_access()`.
- Builder:
  `make_load_local_dynamic_stack_source(prepared, ids, load_local_source_access(ids))`.
- BIR source producer:
  predecessor block label `left` (`ids.predecessor`), instruction index `0`,
  `bir::LoadLocalInst` result `%src`, type `i32`, slot name `dynamic_slot`,
  byte offset `12`, align `4`.
- Prepared row:
  `PreparedMemoryAccess{function_name=ids.function, block_label=ids.predecessor,
  inst_index=0, result_value_name=ids.source_name, address.base_kind=PointerValue,
  address.pointer_value_name=ids.base_name, address.byte_offset=12,
  address.size_bytes=4, address.align_bytes=4,
  address.can_use_base_plus_offset=true}`.
- Lookup identity:
  `find_unique_indexed_prepared_memory_access_by_result_value_id(&lookups.memory_accesses, 1)`
  must return the exact same object as `publication->source_memory_access`.
  The source value id is `1` (`%src`), destination value id is `2` (`%dst`),
  and base value id is `3` (`%base`).
- Publication facts:
  source producer kind `LoadLocal`; source memory access status `Available`;
  source memory base kind `PointerValue`; source memory pointer value name
  `ids.base_name`; byte offset `12`; size `4`; align `4`;
  base-plus-offset available; no address materialization required.

Matching Route 3 / Route 5 authority facts:

- Route 3 is produced from `make_route5_edge_identity_function(dynamic_ids, true)`
  with `bir::route3_build_memory_access_index(route3_predecessor)` and
  `bir::route3_find_memory_access_record(route3_index, 0,
  bir::Route3MemoryAccessNodeKind::LoadLocal)`.
- Matching Route 3 facts are node kind `load_local`, base kind
  `pointer_value`, pointer value `%base`, result value `%src`, byte offset
  `12`, size `4`, align `4`, default address space, non-volatile.
- Route 5 is produced by
  `bir::route5_cfg_edge_publication_record(&route3_predecessor,
  &route3_successor, Value::named(i32, "%dst"), dynamic_ids.destination_name,
  dynamic_ids.source_name)`.
- Matching Route 5 facts are status `memory_source`, predecessor `left`,
  successor `join`, destination `%dst`, source `%src`, source producer kind
  `LoadLocal`, `source_memory_identity_available=true`, and
  `source_memory_access` pointing at the Route 3 LoadLocal instruction with
  byte offset `12` and size `4`.
- Current helper diagnostics mark the positive row with
  `intent.route5_edge_status == MemorySource`,
  `intent.route5_edge_source_agrees == true`, and
  `intent.route3_source_memory_agrees == true`.

Candidate prepared-only fail-closed condition:

- Keep the normal prepared module and its `memory_accesses` row unchanged and
  internally coherent: prepared publication byte offset `12`, source value id
  `1`, base `%base`, `find_unique...(&lookups.memory_accesses, 1)` returns
  `publication->source_memory_access`.
- Build Route 3 / Route 5 evidence through normal BIR route helpers from a
  separate BIR authority fixture whose `LoadLocal` source memory identity is
  non-agreeing, such as byte offset `16` instead of `12`, or whose Route 5
  memory source has `source_memory_identity_available=false`.
- This does not require hand-built stale prepared state. It creates a coherent
  prepared-only row plus absent/non-agreeing semantic route authority. The
  current implementation already exposes the gap: with mismatched or incomplete
  Route 3 memory facts, `consume_edge_publication_move_intent(...)` records
  `route5_edge_source_agrees=false` and
  `route3_source_memory_agrees=false` but still returns `Available` and
  `instruction_text == "lw a1, 12(s2)"`.
- Step 2 should make that same-consumer path fail closed for the selected
  non-agreeing Route 3/Route 5 row while preserving the positive agreement row.

Byte-stable outputs and adjacent contracts to preserve:

- RISC-V exact positive output: `lw a1, 12(s2)` from
  `riscv::emit_prepared_module(prepared)` and from
  `consume_edge_publication_move_intent(...)` when Route 3 / Route 5 agree.
- Positive intent fields: status `Available`, publication pointer preserved,
  source value id `1`, destination value id `2`, source stack slot id `17`,
  no concrete source stack offset, source stack size `4`, align `4`, source
  memory base value id `3`, base register `s2`, memory offset `12`, memory
  size `4`, memory align `4`, destination register `a1`, instruction text
  `lw a1, 12(s2)`.
- Prepared source-memory status names:
  `unavailable`, `available`, `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`.
- Route status/debug names:
  Route 3 node kind `load_local`; Route 3 base kind `pointer_value`; Route 5
  statuses `available`, `memory_source`, `no_match`, and `no_source` must keep
  their existing spellings.
- Existing fallback/status behavior: null lookups -> `MissingSharedLookups`;
  cleared edge publications -> `MissingPublication`; cleared
  `memory_accesses.accesses_by_result_value_id` -> `UnsupportedSourceHome`
  with empty instruction text and no source memory offset; missing prepared
  access -> publication status `missing_prepared_memory_access` and
  `UnsupportedSourceHome`; incomplete prepared access ->
  `incomplete_prepared_memory_access` and `UnsupportedSourceHome`;
  non-LoadLocal source publication -> source memory status `unavailable` and
  ordinary register fallback `mv a1, a0`; materialization-required or non-i32
  dynamic load -> `UnsupportedSourceHome`.
- Helper/oracle rows to preserve:
  `prepared_edge_publication_source_memory_matches_access(...)` accepts the
  matching prepared source memory row and rejects mismatched byte offsets or
  missing result value names; memory-access lookup helpers keep unique
  result-name/result-id lookup, duplicate ambiguity, duplicate list retention,
  and invalid-name rejection.
- Prepared-printer / wrapper / baseline-visible output: this packet found no
  dedicated RISC-V prepared-printer row for this fixture. Preserve existing
  prepared printer diagnostics, route-debug names, wrapper behavior, exact
  target output, and the `backend_riscv_prepared_edge_publication` plus
  `backend_prepared_lookup_helper` CTest names and expected text. Do not weaken
  any baseline strings while adding the fail-closed row.

## Suggested Next

Execute Step 2 from `plan.md`: add the smallest same-consumer RISC-V test row
that keeps the prepared `memory_accesses` row coherent but supplies
non-agreeing or absent Route 3 / Route 5 memory-source authority, then assert
that `consume_edge_publication_move_intent(...)` fails closed for that row
while the positive `lw a1, 12(s2)` agreement path remains available.

## Watchouts

- Do not hide, demote, delete, privatize, wrap, or rename
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken the `lw a1, 12(s2)` compatibility output or adjacent status,
  helper/oracle, fallback, route/debug, prepared-printer, wrapper, exact target,
  or baseline output.
- The current mismatched/incomplete Route 3 rows are diagnostic-only and still
  return `Available`; accepting that as proof would be route drift. The next
  packet should change or assert behavior at the RISC-V same-consumer path, not
  only helper/oracle surfaces.
- Prefer constructing non-agreeing Route 3 / Route 5 facts through normal BIR
  route helper input fixtures instead of manually corrupting prepared lookup
  state or clearing `memory_accesses`.

## Proof

No build or CTest was required or run for this read-only Step 1 fact packet.
No `test_after.log` was created.
