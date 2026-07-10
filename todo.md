Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reprove Representative Integration

# Current Packet

## Just Finished

Completed plan Step 6, `Reprove Representative Integration`.

Reran the focused packed-global coverage and the representative RV64 GCC
torture route for `tests/c/external/gcc_torture/src/pr79737-2.c` from the
committed Step 5 state. The refreshed evidence is under
`build/agent_state/651_step6_representative_reproof/`: `pr79737.hir.txt`,
`pr79737.llvm.txt`, `pr79737.prepared-bir.txt`, `pr79737.o`,
`pr79737.symbols.txt`, and `pr79737.objdump.txt`.

Representative criteria are now satisfied for the source-idea surface:
HIR reports `struct S size=9 align=1`; LLVM lowers the record as
`%struct.S = type <{ [9 x i8] }>` with zero globals in that representation;
ELF symbol evidence records both `i` and `j` as 9-byte `OBJECT GLOBAL`
symbols; prepared BIR records byte-storage aggregate global-symbol accesses at
offsets 0, 2, and 5, all with `range_verdict=proven_in_bounds`; and RV64
object emission completes with `lw`/`sw` access chunks plus bitfield arithmetic
against the packed globals.

No downstream owner remains for the representative packed global object-size
and access mismatch.

## Suggested Next

Execute plan Step 7, `Run Broader Validation And Close Or Park`.

Objective: run the supervisor-selected broader validation for the affected
frontend, LIR/BIR, and RV64 backend scope, then decide whether the source idea
can close or needs a precise park/split decision.

## Watchouts

- The fix is intentionally limited to `#pragma pack(1)` all-bitfield records
  without base subobjects. Mixed-field, union, base-subobject, or incomplete
  packed-layout shapes stay on existing paths.
- The access model uses 4-byte windows for 32-bit declared bitfields. This is
  range-valid under the 9-byte object and keeps Step 3 RV64 object support
  usable; it is not a new generic byte-lane backend implementation.
- Zero-width bitfields and packed objects too small for their declared storage
  window are intentionally excluded from the byte-storage authority until a
  future packet models them correctly.
- Step 7 should provide broader validation before lifecycle close; this Step 6
  packet is focused representative proof only.

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

The proof used:
`ctest --test-dir build -j --output-on-failure -R
"(packed_global_bitfield|llvm_gcc_c_torture_src_pr79737_2_c)"`, followed by
fresh HIR, LLVM, prepared-BIR, object, symbol, and objdump captures for the
representative.

Representative symbol evidence:
`build/agent_state/651_step6_representative_reproof/pr79737.symbols.txt`
records `i` and `j` as 9-byte `OBJECT GLOBAL` symbols.

Representative prepared/object evidence:
`build/agent_state/651_step6_representative_reproof/pr79737.prepared-bir.txt`
records byte-storage aggregate authority for 4-byte accesses at offsets 0, 2,
and 5, all `range_verdict=proven_in_bounds`, and
`build/agent_state/651_step6_representative_reproof/pr79737.objdump.txt`
records RV64 `lw`/`sw` chunks plus bitfield arithmetic against the packed
globals.
