# Current Packet

Status: Active
Source Idea Path: ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Stack Source Path

## Just Finished

Completed Step 1 inventory for idea 25. The current RISC-V prepared
edge-publication consumer lives in `src/backend/mir/riscv/codegen/emit.cpp`
and is exposed through `consume_edge_publication_move_intent`,
`append_edge_publication_move_instruction`, and `emit_prepared_module`.
It already uses shared `PreparedFunctionLookups::edge_publications` as the
only authority via `find_unique_indexed_prepared_edge_publication`; it accepts
`Register -> Register` as `mv <dst>, <src>` and
`RematerializableImmediate -> Register` as `li <dst>, <imm>`.

`StackSlot -> Register` currently reaches
`render_edge_publication_source_operand` and returns
`UnsupportedSourceHome`. The implementation target should stay RISC-V-local:
extend the source-home rendering/emission path in
`src/backend/mir/riscv/codegen/emit.cpp` and the intent shape in
`src/backend/mir/riscv/codegen/emit.hpp` only as needed, then add focused
coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

Proposed first stack-source emission policy: after the shared publication is
accepted and the destination home is confirmed as a register, require a
`PreparedValueHomeKind::StackSlot` source with `offset_bytes` and a register
destination, then emit a RISC-V stack load directly into the destination
register. The focused idea-25 case should use the prepared I32 fixture and
emit `lw <dst>, <offset>(sp)`. If the source home lacks `offset_bytes`, if the
load width is not the focused I32/4-byte case, or if the destination is not a
register, the helper should continue to fail closed rather than guessing a
frame/address policy. `slot_id` should be preserved on the intent/test path as
structured provenance, but final assembly uses the prepared stack offset.

## Suggested Next

Execute Step 2 by implementing `StackSlot -> Register` register-destination
consumption through the existing shared lookup path. Add a positive test that
mutates the current prepared edge fixture source home to a stack slot with a
slot id, offset, and I32-sized payload, then expects `lw a1, <offset>(sp)`.
Keep existing register and immediate cases green.

## Watchouts

Do not scan predecessor/successor blocks or reconstruct edge facts locally.
The shared `edge_publications` lookup remains the semantic authority, and
missing shared lookup authority or a missing publication must still return the
existing fail-closed statuses. Keep `PointerBasePlusOffset -> Register` and
all source-to-`StackSlot` destinations unsupported/fail-closed. Do not broaden
this packet into full RISC-V frame lowering; large offsets, non-I32 load
widths, dynamic stack, and stack destinations need separate ownership unless
the next packet explicitly scopes them.

## Proof

Inventory-only packet; no build or tests run, and no `test_after.log` was
created.

Focused proof command for the implementation packet:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`
