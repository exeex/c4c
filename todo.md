Status: Active
Source Idea Path: ideas/open/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Dynamic Stack-Source Authority

# Current Packet

## Just Finished

Step 1 inventoried RISC-V dynamic-address `StackSlot -> Register` prepared
edge-publication authority.

Current concrete behavior:
- RISC-V edge-publication lowering is owned by
  `src/backend/mir/riscv/codegen/emit.cpp`.
- `consume_edge_publication_move_intent` only consumes a shared
  `PreparedEdgePublication` found by predecessor, successor, and destination
  value id. Clearing `publications_by_edge_destination` still yields
  `MissingPublication`, so the helper does not rediscover edge moves locally.
- Register, rematerializable I32 immediate, concrete `StackSlot`, and
  `PointerBasePlusOffset` source homes are parsed in
  `render_edge_publication_source_operand`.
- Concrete `StackSlot -> Register` sources require
  `PreparedValueHomeKind::StackSlot`, `offset_bytes`, and `size_bytes` of 4 or
  8. Size 4 emits `lw`; size 8 emits `ld`.
- Concrete stack offsets that fit signed 12-bit load immediates use
  `lw/ld <dst>, <offset>(sp)`.
- Large concrete stack offsets use target-local address materialization:
  `li t6, <offset>; add t6, sp, t6; lw/ld <dst>, 0(t6)`.
- Existing stack-destination support is narrower: only non-aliasing I32
  concrete stack-source-to-stack-destination moves with signed-12-bit offsets
  are accepted, using `t0` as a target-local scratch. Pointer sources and large
  stack-source offsets to stack destinations intentionally remain fail closed.

Available prepared facts for this route:
- `PreparedEdgePublication` provides predecessor/successor labels,
  destination/source BIR values, destination/source value ids, source producer
  kind and optional producer pointer, source/destination homes, move phase,
  carrier kind, parallel-copy metadata, and the matching prepared move.
- `PreparedValueHome` for `StackSlot` can provide `slot_id`, `offset_bytes`,
  `size_bytes`, and `align_bytes`.
- `PreparedValueHome` for `Register` can provide raw `register_name` and
  optional target register identity.
- `PreparedValueHome` for `PointerBasePlusOffset` can provide
  `pointer_base_value_name` and `pointer_byte_delta`; RISC-V resolves the base
  value through prepared value-home lookups and accepts it only when the base
  home is a register.
- Shared source producer facts can identify `LoadLocal`, `Cast`, `Binary`,
  `SelectMaterialization`, or `Immediate` producers, but they do not by
  themselves publish a dynamic stack-source memory address for edge loads.

Missing dynamic stack-source authority:
- There is no distinct prepared `StackSlot` dynamic-address home. A `StackSlot`
  without `offset_bytes` is currently just malformed for RISC-V edge loads and
  fails closed as `UnsupportedSourceHome`.
- No edge-publication fact connects a `StackSlot` source home to a dynamic base
  anchor, dynamic offset expression, runtime address value, or address
  materialization record.
- `PreparedMemoryAccess` can describe pointer-value memory accesses elsewhere,
  but `PreparedEdgePublication` does not currently carry the source memory
  access needed to turn an edge source into a dynamic load.
- `PointerBasePlusOffset` is a pointer/address value materialization path for
  edge moves (`mv`/`addi`/`li; add` into the destination register). It is not a
  `StackSlot -> Register` load and does not carry source load width for a
  dynamic memory load.
- Scratch policy for a dynamic stack-source load is absent: no prepared fact
  reserves or authorizes a temporary address register for a dynamic load beyond
  the existing concrete large-offset `t6` convention.

Focused test handles already covering existing and neighboring behavior:
- `backend_riscv_prepared_edge_publication`
- `backend_prepared_lookup_helper`
- In `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`:
  concrete I32 stack-source, concrete I64 stack-source, large-offset I32/I64
  stack-source, stack sources without offsets, malformed stack sources,
  pointer-base source materialization, missing pointer-base fields, unresolved
  pointer-base values, non-register pointer bases, and stack-destination
  unsupported pointer/large-offset cases.

Recommended first policy packet:
- Preserve and make explicit the fail-closed policy for dynamic-address
  `StackSlot -> Register` sources because no shared prepared authority
  currently identifies a dynamic stack-source base anchor, offset expression,
  source memory access, load width, and scratch-register contract.
- Add or tighten focused negative coverage only as needed to prove that
  `StackSlot` homes without concrete offsets remain `UnsupportedSourceHome`
  and that `PointerBasePlusOffset` remains pointer-value materialization, not a
  dynamic stack-source load.
- Do not implement a dynamic load until shared prepared authority publishes the
  dynamic source address and scratch policy.

## Suggested Next

Proceed to Step 2 as a fail-closed policy packet. The likely owned surface is
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` and, only
if needed for an explicit guard/diagnostic, `src/backend/mir/riscv/codegen/emit.cpp`.

Focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

## Watchouts

Do not infer dynamic stack-source addresses from block shape, predecessor or
successor scans, fixture labels, value ids, stack slot ids, offsets, or test
names. Preserve existing concrete 4-byte, 8-byte, and large-offset behavior.
Do not treat `PointerBasePlusOffset` as a dynamic `StackSlot` load; it is
currently an address-value materialization path.

## Proof

Inventory-only packet; no build required.

Read-only commands used:
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `rg -n "PreparedEdgePublication|PreparedValueHome|StackSlot|PointerBasePlusOffset|source_stack|stack_source|edge_publication|consume_edge_publication|UnsupportedSourceHome|lw|ld" src/backend/mir/riscv src/backend -g'*.cpp' -g'*.hpp'`
- targeted `sed` reads of `emit.cpp`, prepared value-home/edge-publication
  structures, prepared lookup construction, and focused RISC-V tests.

`git diff --check -- todo.md` passed.
