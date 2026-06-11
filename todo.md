Status: Active
Source Idea Path: ideas/open/202_route3_memory_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Route 3 Identity Adapter

# Current Packet

## Just Finished

Step 2 added a bounded Route 3 identity adapter for
`make_unpublished_load_local_source_operand(...)` in
`src/backend/mir/aarch64/codegen/alu.cpp`. The reader now requires
`mir::find_bir_same_block_load_local_source_identity(...)` to agree with the
prepared load-local source on the BIR `load_local` instruction, prepared memory
access instruction index, result/root value identity, local-slot memory-access
kind, and frame-slot prepared-addressability before using the prepared memory
operand.

Prepared facts remain authoritative for target addressing: stack layout,
frame-slot id to target address, byte offset, size/align, base-plus-offset
legality, `byte_offset_is_prepared_snapshot`, value homes, and final
`MemoryOperandSupportKind::Prepared` operand formation still come from the
prepared lookup and `make_prepared_scalar_load_source(...)`.

Files changed:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
- `todo.md`

## Suggested Next

Select the next Step 3 packet from `plan.md`: either broaden reader-boundary
coverage for additional fail-closed cases that are still within the selected
AArch64 reader, or have the supervisor decide whether the current Route 3
adapter evidence is sufficient for review/closure.

## Watchouts

Do not move address formation, frame/global/TLS relocation, stack/frame
offsets, concrete layout, addressing-mode legality, materialization, final
operands, or target-addressing fallback into BIR schema.

Do not replace all `memory_accesses`, `PreparedMemoryAccessLookups`, or
memory/frame/stack helper groups.

The focused AArch64 fixture now gives the selected `load_local` instructions
BIR local-slot addresses so Route 3 can answer identity. The mismatch coverage
uses a prepared access instruction-index disagreement and verifies the reader
rejects the source-home operand instead of taking BIR as target-addressing
authority.

## Proof

Supervisor-selected proof command run exactly:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_scalar_alu_records|backend_store_source_publication_plan|backend_prepared_lookup_helper)$' > test_after.log`

Result: passed. `test_after.log` contains 3/3 passing tests:
`backend_store_source_publication_plan`,
`backend_aarch64_prepared_scalar_alu_records`, and
`backend_prepared_lookup_helper`.

Additional validation: `git diff --check` passed.
