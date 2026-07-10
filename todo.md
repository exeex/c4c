Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Run Broader Validation And Close Or Park

# Current Packet

## Just Finished

Completed plan Step 7, `Run Broader Validation And Close Or Park`.

Ran a matched broader regression guard around the substantive Step 5/6 changes.
The before checkpoint was `e1770abce` (`[plan] repair packed bitfield layout
runbook`), and the after checkpoint was current `main` after
`038fb771f` (`[todo_only] record packed global representative reproof`).

Both sides used the same command shape with backend enabled:
`ctest --test-dir build -j --output-on-failure -R
"^(frontend_hir_tests$|positive_sema_|backend_|llvm_gcc_c_torture_src_pr79737_2_c$)"`.
This broader scope covers HIR, positive C sema, the backend/LIR/BIR/RV64
buckets, and the representative RV64 GCC torture case.

`c4c-regression-guard` passed:
before `366/398` passed with 32 known failures; after `369/401` passed with
the same 32 known failures. The delta was `+3` passing tests, `0` new failures,
and `0` newly slow tests. The added passing tests are the focused HIR
packed-global bitfield coverage introduced by Step 5.

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

Request plan-owner close for
`ideas/open/651_rv64_packed_bitfield_global_layout_access.md`.

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
- No park/split owner remains for the source-idea acceptance criteria after
  Step 7 validation.

## Proof

`test_before.log` and `test_after.log` are the matched broader regression
guard logs. The guard command was:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Guard result: `PASS`.

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
