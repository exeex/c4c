Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Integration

# Current Packet

## Just Finished

Completed plan Step 4, `Prove Representative Integration`.

Reran focused packed-global coverage plus the RV64 GCC torture representative
route for `tests/c/external/gcc_torture/src/pr79737-2.c`. The representative
CTest route now passes, and C4C emits a RV64 object for the file. Evidence is
under `build/agent_state/651_step4_representative_integration/`:
`pr79737.o`, `pr79737.symbols.txt`, `pr79737.objdump.txt`, and
`pr79737.prepared-bir.txt`. The same directory also contains producer-side
layout probes `pr79737.hir.txt` and `pr79737.llvm.txt`.

The representative has not advanced through the packed object-size mismatch:
ELF symbol evidence still reports globals `i` and `j` as 12-byte objects. The
prepared access evidence still uses 4-byte global-symbol loads/stores at
offsets 0, 4, and 8 with `layout_authority=byte_storage_aggregate` and
`range_verdict=proven_in_bounds`, and objdump confirms RV64 `lw`/`sw` chunks
rather than byte-lane packed accesses.

No implementation change was made in this packet because the remaining owner
is not a narrow RV64 object-writer choice. The producer surface still models
the representative as a 12-byte packed aggregate before RV64 object data:
`--dump-hir` reports `struct S size=12 align=1`, and the LIR/LLVM route emits
`%struct.S = type <{ i32, i32, i32 }>` with 12-byte globals. Forcing 9-byte
ELF symbols here would outrun the semantic layout and would make the existing
offset-8 4-byte access authority inconsistent.

## Suggested Next

Execute a narrow layout/access producer packet for packed bitfield aggregates:
teach the HIR-to-LIR/BIR layout path to represent the representative
`#pragma pack(1)` all-bitfield aggregate as a semantic 9-byte object and lower
its bitfield accesses through byte-lane or otherwise range-valid packed
accesses. Keep it fact-driven and add focused coverage before returning to RV64
object emission proof.

## Watchouts

- The remaining mismatch is upstream of RV64 object data selection:
  `src/frontend/hir/hir_types.cpp` currently assigns three 32-bit bitfield
  storage units and reports `struct S size=12 align=1`.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` then renders that as
  `%struct.S = type <{ i32, i32, i32 }>`; BIR globals and prepared object data
  inherit the 12-byte extent.
- A valid 9-byte fix must also adjust access lowering. Keeping the current
  4-byte offset-8 loads/stores while shrinking the object to 9 bytes would
  conflict with range authority.
- Keep the next packet semantic and layout-driven. Do not special-case
  `pr79737-2.c`, `i`, `j`, source spelling, expectations, unsupported markers,
  allowlists, timeouts, or pass/fail accounting.

## Proof

Exact delegated proof command ran and passed. `test_after.log` is the proof
log.

Focused CTest subset passed 3/3:
`backend_dump_riscv64_packed_global_bitfield_access`,
`backend_cli_riscv64_packed_global_bitfield_access`, and
`llvm_gcc_c_torture_src_pr79737_2_c`.

Representative symbol evidence:
`build/agent_state/651_step4_representative_integration/pr79737.symbols.txt`
records `i` and `j` as 12-byte `OBJECT GLOBAL` symbols, not 9-byte objects.

Representative prepared/object evidence:
`build/agent_state/651_step4_representative_integration/pr79737.prepared-bir.txt`
records byte-storage aggregate authority for 4-byte accesses at offsets 0, 4,
and 8, and
`build/agent_state/651_step4_representative_integration/pr79737.objdump.txt`
records RV64 `lw`/`sw` chunks plus bitfield arithmetic.
