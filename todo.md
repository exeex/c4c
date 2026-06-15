Status: Active
Source Idea Path: ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Identify Byte-Offset Drift Row Facts

# Current Packet

## Just Finished

Step 1: Identified the byte-offset drift row facts for the RISC-V
same-consumer `PreparedFunctionLookups::memory_accesses` proof before any
code/test edits.

- Backend consumer: `render_edge_publication_source_operand(...)` in
  `src/backend/mir/riscv/codegen/emit.cpp` directly reads
  `PreparedFunctionLookups::memory_accesses` through
  `find_unique_indexed_prepared_memory_access_by_result_value_id(
  &lookups->memory_accesses, *publication.source_value_id)`. The public entry
  path is `consume_edge_publication_move_intent(..., route5_edge)` through
  `RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent()` and
  `RiscvEdgePublicationMoveAdapter::render_prepared_source_operand(...)`.
- Concrete prepared row: the dynamic stack-source fixture
  `make_load_local_dynamic_stack_source(prepared, ids,
  load_local_source_access(ids))` builds `LoadLocal %src` from
  `dynamic_slot`, `inst_index=0`, `byte_offset=12`, `align_bytes=4`. Its normal
  prepared row is `PreparedMemoryAccess{function=join_regs, block=left,
  inst_index=0, result_value_name=%src, address.base_kind=PointerValue,
  pointer_value_name=%base, byte_offset=12, size_bytes=4, align_bytes=4,
  can_use_base_plus_offset=true}`. Normal prepared lookup construction exposes
  this row by source value id `1`, and the indexed row is the same pointer as
  `publication->source_memory_access`.
- Positive Route 3 memory/source authority fact: the supported BIR route is
  `make_route5_edge_identity_function(dynamic_ids, true)`, then
  `route3_build_memory_access_index(route3_predecessor)` and
  `route3_find_memory_access_record(route3_index, 0, LoadLocal)`. The row has
  `node_kind=load_local`, `base_kind=pointer_value`, pointer value `%base`,
  `byte_offset=12`, `size_bytes=4`; the matching Route 5 wrapper row has
  `status=MemorySource` / public name `"memory_source"`,
  `source_memory_identity_available=true`, and its `source_memory_access`
  instruction, byte offset, and size match the Route 3 row and prepared
  publication.
- Offset facts: prepared byte offset is `12`; positive Route 3 byte offset is
  `12`; chosen drift Route 3 byte offset is `16`, produced by changing only
  `mismatched_route3_memory_edge.source_memory_access.byte_offset = 16` while
  the normal prepared lookup row remains present.
- Expected fail-closed result for the drift row:
  `intent.status == EdgePublicationMoveIntentStatus::UnsupportedSourceHome`,
  `intent.instruction_text.empty()`,
  `intent.route5_edge_status == Route5PublicationStatus::MemorySource`,
  `intent.route5_edge_source_agrees == false`, and
  `intent.route3_source_memory_agrees == false`.
- Stable public compatibility outputs/facts: positive target output remains
  `lw a1, 12(s2)` from `emit_prepared_module(prepared)` and from
  `intent.instruction_text`; positive helper facts remain `source_value_id=1`,
  `destination_value_id=2`, `source_memory_base_value_id=3`,
  `source_memory_base_register="s2"`, `source_memory_byte_offset=12`,
  `source_memory_size_bytes=4`, `source_memory_align_bytes=4`,
  `destination_register="a1"`, `route5_edge_source_agrees=true`, and
  `route3_source_memory_agrees=true`. The public fallback behavior without a
  Route 3 diagnostic row remains `Available` with `lw a1, 12(s2)`. Adjacent
  status strings that must stay stable include `"memory_source"`,
  `"load_local"`, `"pointer_value"`,
  `"missing_prepared_memory_access"`, and
  `"incomplete_prepared_memory_access"`. Prepared-printer addressing text for
  the row format remains `access block=<label> inst_index=<n>
  base=pointer_value result=<value> pointer=<value> offset=12 size=4 align=4
  base_plus_offset=yes`.
- Blocker check: no blocker found for Step 1. The supported fixture can express
  the byte-offset drift row without hand-built stale prepared state by keeping
  the prepared lookup construction normal and mutating only the Route 3
  diagnostic memory-source byte offset.

## Suggested Next

Execute Step 2: add or expose the selected byte-offset drift row through the
supported RISC-V fixture/oracle path, keeping the positive `lw a1, 12(s2)` path
and normal prepared lookup row intact.

## Watchouts

- The byte-offset-only drift is Route 3 `16` versus prepared `12`; do not
  change the prepared memory row, `LoadLocal` identity, pointer base `%base`,
  size/alignment, value ids, or public RISC-V output to make the row pass.
- The real same-consumer fail-closed gate is
  `route3_source_memory_agrees_with_prepared_publication(...)`, which compares
  Route 3 `byte_offset` to `publication.source_memory_byte_offset` and causes
  the RISC-V consumer to return `UnsupportedSourceHome` when the Route 5 memory
  source row is present but non-agreeing.
- Do not hide, demote, delete, privatize, wrap, or rename
  `PreparedFunctionLookups::memory_accesses`.
- Do not weaken the `lw a1, 12(s2)` compatibility output or adjacent status,
  helper/oracle, fallback, route/debug, prepared-printer, wrapper, exact target,
  or baseline output.

## Proof

No build or test proof was required by the delegated packet. Fact-finding used
`c4c-clang-tool-ccdb` symbol/callee queries for the RISC-V backend consumer and
fixture functions, plus scoped `rg`/snippet reads for nearby expected strings.
No `test_after.log` was produced because the packet explicitly required no
build or test proof.
