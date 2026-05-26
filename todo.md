Status: Active
Source Idea Path: ideas/open/40_riscv_prepared_edge_publication_typed_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Typed Stack-Source Authority

# Current Packet

## Just Finished

Step 1 inventoried RISC-V typed scalar `StackSlot -> Register` prepared
edge-publication authority.

Current RISC-V edge-publication lowering is concentrated in
`src/backend/mir/riscv/codegen/emit.cpp`:
`consume_edge_publication_move_intent` consumes
`find_unique_indexed_prepared_edge_publication`, then
`render_edge_publication_source_operand` records source home facts into
`EdgePublicationMoveIntent`. Current concrete stack-source behavior accepts
only `PreparedValueHomeKind::StackSlot` source homes with `offset_bytes` and
`size_bytes` equal to 4 or 8. Register destinations emit `lw` for size 4 and
`ld` for size 8. Large register-destination source offsets materialize through
`li t6; add t6, sp, t6; lw/ld ..., 0(t6)`. Stack destinations support only
non-aliasing size-4 stack sources with signed-12-bit source and destination
offsets through `lw t0; sw t0`; pointer sources and large stack-source offsets
to stack destinations remain fail closed.

Prepared facts available to RISC-V today:
- source stack slot identity: `PreparedValueHome::slot_id`
- source stack offset: `PreparedValueHome::offset_bytes`
- source byte width: `PreparedValueHome::size_bytes`
- source alignment: `PreparedValueHome::align_bytes`, though RISC-V edge
  publication does not consume it today
- source and destination BIR values and `TypeKind`:
  `PreparedEdgePublication::source_value` and `destination_value`
- source/destination prepared value ids and names
- source/destination home kind and home pointer
- source producer kind and optional producer instruction pointers:
  `source_load_local`, `source_cast`, `source_binary`, and `source_select`
- destination register spelling from `PreparedValueHome::register_name`
- optional `PreparedValueHome::target_register_identity`, but current normal
  edge-publication fixtures and value homes do not provide this as a reliable
  register-bank/view contract

Missing authority for the candidate typed forms:
- signed sub-word stack sources need a signedness/extension fact to select
  `lb` or `lh`; `TypeKind::I8`/`I16` records width but not signedness
- unsigned sub-word stack sources need a signedness/extension fact to select
  `lbu` or `lhu`; `TypeKind::I8`/`I16` records width but not unsignedness
- unsigned I32 stack sources need an unsigned-extension fact to select `lwu`
  instead of current `lw`; `TypeKind::I32` alone is not signedness authority
- F32 stack sources need explicit destination register-bank/view authority
  before selecting `flw`; raw register spelling is not enough, and current
  RISC-V edge lowering does not validate FPR destination homes

Focused tests already covering existing and neighboring unsupported behavior
live in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.
Existing supported behavior:
`check_stack_slot_to_register_move_uses_shared_lookup`,
`check_stack_slot_i64_to_register_move_uses_shared_lookup`,
`check_large_offset_stack_slot_to_register_loads_use_shared_lookup`, and
`check_stack_slot_to_stack_slot_i32_move_uses_shared_lookup`.
Neighboring fail-closed behavior:
`check_stack_source_fail_closed_forms`,
`check_stack_destination_fail_closed_forms`,
`check_pointer_base_fail_closed_forms`, and
`check_missing_and_unsupported_homes_fail_closed`.

Decision: no typed scalar candidate is safe to implement in RISC-V yet. The
exact blocker is missing prepared typed stack-source authority for signedness
and destination register bank/view. Implementing `lb`/`lbu`/`lh`/`lhu`/`lwu` or
`flw` directly from size, BIR `TypeKind`, or register spelling would violate
the source idea's reject signals.

## Suggested Next

Recommended next packet: preserve fail-closed behavior and make the typed
blocker explicit in RISC-V tests without adding support. Add focused negative
coverage showing sub-word, unsigned-I32-shaped, and F32-shaped stack-source
fixtures remain unsupported without explicit prepared signedness/register-bank
authority. Do not emit new typed load opcodes yet.

Focused proof for that packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

## Watchouts

The existing shared `PreparedEdgePublication` has useful BIR value and source
producer pointers, but not a normalized typed stack-source load policy. Do not
treat source producer shape, value ids, stack offsets, fixture names, or raw
register names as signedness or register-bank authority. A future support
packet needs a shared prepared fact such as explicit stack-source extension
kind and destination register bank/view before target-local opcode selection.

## Proof

No build required for this inventory-only packet. Read-only proof commands run:

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`

`rg -n "backend_riscv|riscv_prepared|prepared_edge_publication|large_offset|StackSlot -> Register|source_stack" tests/backend src/backend -g'*.cpp' -g'CMakeLists.txt'`

`rg -n "struct PreparedValueHome|enum class PreparedValueHomeKind|struct PreparedEdgePublication|struct PreparedMoveResolution|struct PreparedFunctionLookups|source_home|destination_home|source_value_id|destination_value_id" src/backend/prealloc -g'*.hpp'`

Result: inventory complete. No implementation files, tests, build files,
`plan.md`, or source idea files edited.
