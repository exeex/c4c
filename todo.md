Status: Active
Source Idea Path: ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Aggregate Stack-Source Authority

# Current Packet

## Just Finished

Step 1 inventory completed for RISC-V aggregate-width `StackSlot -> Register`
prepared edge-publication authority.

Current concrete stack-source behavior:

- `src/backend/mir/riscv/codegen/emit.cpp` consumes only shared
  `PreparedEdgePublication` lookup records for edge-publication moves; clearing
  the lookup map yields `MissingPublication` in the focused tests.
- `StackSlot -> Register` source rendering is limited to stack homes with a
  concrete `offset_bytes` and `size_bytes` equal to 4 or 8.
- Concrete 4-byte stack sources emit `lw`; concrete 8-byte stack sources emit
  `ld`.
- Large concrete offsets for 4-byte and 8-byte stack sources use the existing
  `t6` materialization sequence (`li t6`, `add t6, sp, t6`, then `lw`/`ld`
  from `0(t6)`).
- Missing concrete offsets, sub-word widths, typed/floating mismatches, and
  aggregate-width stack sources are fail-closed as `UnsupportedSourceHome`.

Available prepared facts:

- Source identity: `PreparedEdgePublication` carries predecessor/successor,
  source/destination value ids and names, source producer kind, publication
  phase, edge-transfer metadata, and move-resolution metadata.
- Source stack slot: `PreparedValueHome` carries `kind`, `slot_id`,
  `offset_bytes`, `size_bytes`, and `align_bytes`; frame slots also carry slot
  size/alignment.
- Destination register: destination homes and moves carry
  `register_name`, optional `target_register_identity`,
  `destination_register_name`, `destination_contiguous_width`, occupied
  register names, and optional destination register placement.
- Register vocabulary includes `PreparedRegisterBank::AggregateAddress` and
  contiguous-width placement data, but the RISC-V edge-publication emitter only
  has scalar `lw`/`ld` stack-source policy.

Missing authority for aggregate-width support:

- No edge-publication record states an aggregate copy width distinct from raw
  source home `size_bytes`, selected lanes, lane offsets, or partial-copy
  rules.
- No prepared edge source memory-access record provides aggregate load/store
  sequencing, ABI layout, or alignment policy for the copy.
- No destination-lane contract maps an aggregate stack source to one or more
  destination registers or aggregate-address registers.
- No scratch ownership/reservation exists for aggregate stack-source copies;
  current `t6` use is only the scalar large-offset load helper, and current
  `t0` use is only the narrow stack-destination helper.

Focused tests and handles:

- `backend_riscv_prepared_edge_publication` covers existing 4-byte, 8-byte,
  and large-offset stack-source behavior, missing shared-publication lookup
  failures, unsupported typed/dynamic stack-source forms, and the current
  aggregate-width fail-closed case.
- `backend_prepared_lookup_helper` is the neighboring shared lookup guard for
  prepared edge-publication authority.

Recommended first policy packet:

- Preserve and make explicit the aggregate-width fail-closed policy for
  `StackSlot -> Register` until shared prepared authority supplies aggregate
  copy width, lane/destination, alignment/partial-copy, ABI layout, and scratch
  facts.
- Tighten focused tests as needed so aggregate-width stack sources cannot be
  accepted through scalar `lw`/`ld`, large-offset `t6` scalar-load sequences,
  raw register spelling, stack slot ids, value ids, offsets, or fixture names.
- Do not add aggregate load support in this route packet; the current blocker
  is missing prepared aggregate copy authority, not an emitter-only omission.

## Suggested Next

Run Step 2 as a fail-closed policy packet: add or tighten focused
`backend_riscv_prepared_edge_publication` tests proving aggregate-width
`StackSlot -> Register` sources remain `UnsupportedSourceHome`, no scalar
`lw`/`ld` or large-offset scalar-load sequence is emitted for them, and existing
4-byte, 8-byte, and large-offset scalar stack-source behavior remains green.
Only touch RISC-V edge-publication lowering if the tests expose an implicit
acceptance path that needs an explicit guard/diagnostic.

## Watchouts

The aggregate policy blocker is specific: shared prepared edge-publication data
does not yet describe aggregate copy width, lanes, partial-copy/alignment/ABI
layout, destination lane mapping, or scratch ownership. Do not treat aggregate
stack sources as scalar loads based only on size, fixture names, value ids,
stack slot ids, offsets, or register spelling. Preserve existing scalar 4-byte,
8-byte, and large-offset behavior and do not broaden into typed, dynamic,
pointer, or source-to-stack edge-publication routes.

## Proof

No build required for inventory-only packet.

Read-only inventory commands used:

- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `rg -n "PreparedMoveResolution|PreparedMoveStorageKind|PreparedTargetRegisterIdentity|AggregateAddress|contiguous_width|target_register_identity|partial|lane|aggregate|scratch" src/backend/prealloc src/backend/mir/riscv/codegen tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp -g'*.hpp' -g'*.cpp'`
- `rg -n "aggregate-width|StackSlot.*16|set_stack_source\([^\n]*(16|32)|source_stack_size_bytes|0\(t6\)|MissingPublication|UnsupportedSourceHome" tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Recommended focused proof for the next packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`
