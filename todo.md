Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Layout And Access Boundary

# Current Packet

## Just Finished

Completed plan Step 2, `Locate The Layout And Access Boundary`, for
`tests/c/external/gcc_torture/src/pr79737-2.c`.

Diagnostic summary:
`build/agent_state/651_step2_packed_global_boundary/summary.md`.

Boundary classification: RV64 prepared object-emission consumer. Step 1
evidence already has prepared direct global-symbol access facts for the packed
bitfield storage units: `layout_authority=byte_storage_aggregate`,
`range_verdict=proven_in_bounds`, `base_plus_offset=yes`, `size=4`,
`align=4`, and exact result/stored value identity. The first stop is the
dependent i32 bitfield extraction chain at `main`, `entry`,
`instruction_index=4`, `BinaryInst`, owner `%t1.bf.mask`, after
`%t1.bf.unit = bir.load_global i32 @i`.

Owned implementation surface for Step 3:
`src/backend/mir/riscv/codegen/object_emission.cpp` around
`fragment_for_prepared_instruction(...)`, `fragment_for_prepared_binary(...)`,
`fragment_for_prepared_narrow_bitfield_binary(...)`, and
`prepared_memory_access_for_instruction(...)`, plus the prepared-global helper
surface in `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
around `prepared_global_access_is_supported(...)`,
`fragment_for_prepared_load_global(...)`, and
`fragment_for_prepared_store_global(...)`.

Focused positive proof shape: add RV64 backend coverage for a packed
file-scope bitfield aggregate whose semantic object size is not a word-lane
multiple, prove prepared byte-storage aggregate global access facts, prove
object emission for the focused testcase and `src/pr79737-2.c`, and verify the
representative globals emit as 9-byte packed objects rather than 12-byte
word-lane objects once object output is available.

Fail-closed negative shape: keep object emission unsupported for missing
packed layout facts, non-global-symbol or non-default address-space accesses,
volatile accesses, incomplete or unproven requested ranges, missing/inexact
result or stored-value identity, unsupported bitfield spans, unsupported
storage unit size/alignment, mismatched object storage size, or missing
prepared GPR/stack homes for the dependent bitfield arithmetic.

Step 3 should implement, not split. The boundary is local to RV64 prepared
global access/object emission and is keyed to semantic prepared facts rather
than the filename, globals `i`/`j`, exact source spelling, or pass/fail
accounting.

## Suggested Next

Execute Step 3: implement the narrow packed-global i32 bitfield-chain consumer
for RV64 prepared object emission, with focused positive and fail-closed
negative tests.

## Watchouts

- Keep the implementation keyed to prepared access authority and value homes,
  not representative names or exact instruction indexes.
- Do not treat advancing past `%t1.bf.mask` alone as sufficient; the focused
  proof should cover both packed-global reads and read-modify-write stores.
- Object layout proof is still pending until emission advances. Step 4 should
  verify the representative globals as 9-byte packed objects.
- Preserve fail-closed behavior for incomplete packed layout/access authority
  instead of silently accepting generic word-lane lowering.

## Proof

Diagnostic-only packet. No broad tests were rerun and `test_after.log` was not
modified. Used existing Step 1 evidence under
`build/agent_state/651_step1_packed_global_evidence/` plus targeted
`c4c-clang-tool-ccdb` symbol/signature queries and focused source reads.
