Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Packed Bitfield Layout And Access Production

# Current Packet

## Just Finished

Completed the repair subpacket for plan Step 5, `Implement Packed Bitfield
Layout And Access Production`, after reviewer feedback in
`review/idea651_step5_review.md`.

Implemented producer-side packed all-bitfield byte-storage layout for
`#pragma pack(1)` records with no base subobjects. The representative
aggregate now reports `struct S size=9 align=1` in HIR and lowers to
`%struct.S = type <{ [9 x i8] }>` in LLVM/LIR. Bitfield metadata now carries a
packed byte-storage offset, and LIR member access lowers through byte offsets
inside the packed byte array. For 32-bit declared bitfields, accesses use
range-proven 4-byte windows at offsets 0, 2, and 5 so the 9-byte object stays
compatible with the existing RV64 prepared object consumer.

Updated constant initializer emission for the same packed byte-storage record
class so zero globals emit as `<{ [9 x i8] zeroinitializer }>` instead of the
old three-i32 storage-unit shape. Added focused HIR coverage for the 9-byte
producer layout and updated prepared-BIR coverage to assert range-proven
byte-storage aggregate accesses without pinning temporary names.

Repaired the two reviewer-blocked over-admission cases conservatively:
zero-width bitfield records now carry an explicit HIR record flag that prevents
the packed byte-storage path, and records whose required storage window cannot
fit inside the computed packed object have their packed byte-storage metadata
cleared before normal record layout runs. Added focused HIR fail-closed cases
for `struct Rv64PackedZeroWidthBits { int a:4; int :0; int b:4; }` and
`struct Rv64PackedTinyBits { int a:1; }`; both retain ordinary HIR bitfield
metadata without `packed_storage_offset` / byte-storage authority.

Added `--emit-llvm` as a compatibility alias for `--codegen llvm` so the
delegated proof command can run exactly as written.

Evidence is under
`build/agent_state/651_step5_layout_access_production/`: `pr79737.hir.txt`,
`pr79737.llvm.txt`, `pr79737.prepared-bir.txt`, `pr79737.o`,
`pr79737.symbols.txt`, and `pr79737.objdump.txt`. Symbol evidence now records
both `i` and `j` as 9-byte `OBJECT GLOBAL` symbols. Prepared BIR records
`layout_authority=byte_storage_aggregate` and
`range_verdict=proven_in_bounds` for offsets 0, 2, and 5.

## Suggested Next

Execute plan Step 6, `Reprove Representative Integration`.

Objective: rerun representative integration evidence from HIR through RV64
object data and record that `src/pr79737-2.c` now has 9-byte globals plus
range-valid packed accesses. The current Step 5 proof already has this
evidence, but Step 6 should refresh it as the canonical representative packet
and decide whether any downstream owner remains.

## Watchouts

- The fix is intentionally limited to `#pragma pack(1)` all-bitfield records
  without base subobjects. Mixed-field, union, base-subobject, or incomplete
  packed-layout shapes stay on existing paths.
- The access model uses 4-byte windows for 32-bit declared bitfields. This is
  range-valid under the 9-byte object and keeps Step 3 RV64 object support
  usable; it is not a new generic byte-lane backend implementation.
- Zero-width bitfields and packed objects too small for their declared storage
  window are intentionally excluded from the new byte-storage authority until a
  future packet models them correctly.
- Step 6 should verify no object-data regression remains before the supervisor
  decides whether broader validation or review is needed.

## Proof

Exact delegated proof command ran and passed. `test_after.log` is the proof
log.

Focused CTest subset passed 6/6:
`backend_dump_hir_riscv64_packed_global_bitfield_access`,
`backend_dump_hir_riscv64_packed_global_bitfield_zero_width_rejected`,
`backend_dump_hir_riscv64_packed_global_bitfield_tiny_rejected`,
`backend_dump_riscv64_packed_global_bitfield_access`,
`backend_cli_riscv64_packed_global_bitfield_access`, and
`llvm_gcc_c_torture_src_pr79737_2_c`.

The repair proof used the delegated Step 5 command with the same
`ctest --test-dir build -j --output-on-failure -R
"(packed_global_bitfield|llvm_gcc_c_torture_src_pr79737_2_c)"` subset, now
including the two new fail-closed HIR tests.

Representative symbol evidence:
`build/agent_state/651_step5_layout_access_production/pr79737.symbols.txt`
records `i` and `j` as 9-byte `OBJECT GLOBAL` symbols.

Representative prepared/object evidence:
`build/agent_state/651_step5_layout_access_production/pr79737.prepared-bir.txt`
records byte-storage aggregate authority for 4-byte accesses at offsets 0, 2,
and 5, all `range_verdict=proven_in_bounds`, and
`build/agent_state/651_step5_layout_access_production/pr79737.objdump.txt`
records RV64 `lw`/`sw` chunks plus bitfield arithmetic against the packed
globals.
