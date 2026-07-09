Status: Active
Source Idea Path: ideas/open/651_rv64_packed_bitfield_global_layout_access.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Packed Global Evidence

# Current Packet

## Just Finished

Completed plan Step 1, `Refresh Packed Global Evidence`, for
`tests/c/external/gcc_torture/src/pr79737-2.c`.

Evidence paths:

- `test_after.log`
- `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/case.log`
- `build/agent_state/651_step1_packed_global_evidence/summary.md`
- `build/agent_state/651_step1_packed_global_evidence/pr79737.bir.txt`
- `build/agent_state/651_step1_packed_global_evidence/pr79737.prepared.txt`
- `build/agent_state/651_step1_packed_global_evidence/pr79737.mir.txt`
- `build/agent_state/651_step1_packed_global_evidence/pr79737.obj.log`

Current harness outcome: `src/pr79737-2.c` fails the RV64 GCC torture backend
route with `total=1 passed=0 failed=1`. The C4C object route stops before
object emission with `unsupported_instruction_fragment` in `main`, `entry`,
`instruction_index=4`, `instruction_kind=BinaryInst`, owner `%t1.bf.mask`.

Packed global facts recorded from the dumps: source globals `i` and `j` are
the `#pragma pack(1)` file-scope bitfield aggregate surface. Semantic and
prepared BIR currently use `bir.load_global i32` / `bir.store_global` at
offsets `0`, `4`, and `8` for both globals. Prepared addressing records these
as `base=global_symbol`, `layout_authority=byte_storage_aggregate`,
`range_verdict=proven_in_bounds`, `size=4`, and `align=4`.

Object/symbol facts: no C4C object, symbol table, or objdump exists for this
case yet because C4C does not emit `pr79737.c4c.o`; the harness case directory
contains only `case.log`. No `lw`/`sw` versus `lbu`/`sb` object evidence is
available until this object-route stop advances.

First owner classification: access lowering. The current first observable
owner is RV64 lowering of the packed-global bitfield extraction/update chain
after global i32 unit loads, not missing global-symbol identity. Step 2 should
still preserve the later requirement to verify 9-byte global object emission
once object generation reaches the emission stage.

## Suggested Next

Execute Step 2: locate the narrow packed-global access-lowering boundary for
the global bitfield load/store chain, starting from prepared BIR access records
with `layout_authority=byte_storage_aggregate` and the unsupported
`%t1.bf.mask` owner.

## Watchouts

- Do not special-case `src/pr79737-2.c`, global names `i` or `j`, or exact
  source spelling.
- Do not weaken expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- Do not treat the current i32 lane accesses as final proof of correct packed
  object layout; C4C has not emitted an object or symbol table for this case.
- Preserve fail-closed behavior for incomplete packed layout/access authority
  instead of silently accepting generic word-lane lowering.

## Proof

Ran the exact delegated Step 1 proof command. It configured and built the
backend, ran the RV64 GCC torture backend allowlist containing
`src/pr79737-2.c`, captured BIR/prepared-BIR/MIR/object-failure evidence, and
preserved `test_after.log`.

The proof command exits with status `1` because the refreshed harness outcome
is still the expected current failure for this evidence packet:
`[rv64-gcc-torture] total=1 passed=0 failed=1`.
