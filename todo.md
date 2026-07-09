Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Packed Global Owner

# Current Packet

## Just Finished

Completed plan Step 3, `Implement The Narrow Packed Global Owner`.

Implemented narrow RV64 object-emission support for packed-global i32
bitfield storage-unit load/store consumers and their same-block dependent
bitfield arithmetic chains. The admission rule is keyed to prepared direct
global-symbol memory-access facts with default address space, non-volatile
access, `layout_authority=byte_storage_aggregate`,
`range_verdict=proven_in_bounds`, complete object extent, exact
result/stored-value identity, and 4-byte size/alignment. The arithmetic helper
fires only when the same-block i32 chain is rooted in an admitted packed-global
load and uses prepared homes/rematerializable immediates for operands.

Added focused positive coverage:
`tests/backend/case/riscv64_packed_global_bitfield_access.c`, with
`backend_dump_riscv64_packed_global_bitfield_access` proving prepared
byte-storage aggregate access facts and
`backend_cli_riscv64_packed_global_bitfield_access` proving RV64 object
emission. `tests/c/external/gcc_torture/src/pr79737-2.c` now emits an RV64
object, and Step 3 evidence is under
`build/agent_state/651_step3_packed_global_owner/`.

Focused fail-closed negative coverage could not be expressed as a CTest in
this slice: attempted volatile, non-packed, unaligned, and edge-offset packed
bitfield variants still received complete prepared byte-storage aggregate
authority from the producer side, so they were not valid missing-authority
negative cases for this owner. The implementation remains fail-closed through
the admission predicates above; a future negative needs producer-side fixture
support for malformed or absent prepared access facts.

## Suggested Next

Execute Step 4: prove representative integration and decide the next owner for
the remaining packed object-size/layout mismatch.

## Watchouts

- Step 3 advanced `pr79737-2.c` through RV64 object emission and captured
  disassembly with prepared global `lw`/`sw` plus `slliw`/`srliw`/`sraiw`
  bitfield arithmetic.
- Step 3 did not complete the source idea: symbol evidence still shows
  representative globals `i` and `j` as 12-byte objects, not the desired
  9-byte packed objects. Step 4 should own whether this is an object-data
  selection/layout producer gap or an RV64 data-emission selection gap.
- Keep any next change keyed to layout/access facts, not `pr79737-2.c` or
  globals `i`/`j`.

## Proof

Exact delegated proof command ran and passed. `test_after.log` is the proof
log.

Focused CTest subset:
`backend_dump_riscv64_packed_global_bitfield_access` and
`backend_cli_riscv64_packed_global_bitfield_access`, passed 2/2.

Representative evidence:
`build/agent_state/651_step3_packed_global_owner/pr79737.o`,
`build/agent_state/651_step3_packed_global_owner/pr79737.symbols.txt`, and
`build/agent_state/651_step3_packed_global_owner/pr79737.objdump.txt`.
The symbol evidence currently records `i` and `j` as 12-byte objects.
