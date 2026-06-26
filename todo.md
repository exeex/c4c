Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Lower The First Reusable Instruction Family

# Current Packet

## Just Finished

Step 2 implemented semantic RV64 object lowering for the prepared scalar
`bir.ashr` family in `fragment_for_prepared_binary()`.

Completed work:
- Added `BinaryOpcode::AShr` register-shift lowering through the existing
  prepared binary operand/result-home path: I32 uses RV64 `sraw` and I64 uses
  RV64 `sra`.
- Added prepared-immediate AShr lowering: I32 uses RV64 `sraiw` and I64 uses
  RV64 `srai`, with fail-closed rejection for negative or width-out-of-range
  immediate shift amounts.
- Kept the existing prepared fact contract: unsupported widths, missing homes,
  and non-materializable/non-GPR scalar operands still return `std::nullopt`
  through the RV64 object-route diagnostic path.
- Added focused backend coverage for prepared scalar AShr I32 register and
  immediate shifts, AShr I64 register and immediate shifts, and an invalid I32
  immediate shift amount `32`.

Representative outcomes:
- `src/20020225-2.c` now passes the delegated RV64 gcc-torture backend object
  probe; it no longer fails on scalar `bir.ashr i32 %p.x, %t4`.
- `src/pr48973-2.c` no longer fails with the scalar `bir.ashr ..., 31`
  unsupported-instruction-fragment diagnostic, and its AShr sites now encode as
  `sraiw s1,t3,0x1f`. It still fails at `[RV64_BACKEND_RUNTIME_MISMATCH]`
  with `clang_exit=0` and `c4c_exit=Subprocess aborted`; disassembly shows the
  remaining abort is caused by an earlier global-store source/value clobber:
  after computing the bitfield store value in `t0`, the store-address
  materialization also uses `t0`, copies that address to `t1`, and emits
  `sw t1,0(t0)`.

## Suggested Next

Delegate Step 3 to refresh the instruction-fragment bucket and classify the
next remaining 395-scope family, while treating the `pr48973-2.c` runtime
mismatch as a separate later boundary unless the refreshed bucket says
otherwise.

## Watchouts

- Do not absorb stack-frame or parameter-home work owned by idea 398.
- Do not infer missing producer facts inside RV64 object emission.
- Do not claim progress from filename-specific lowering, expectation rewrites,
  allowlist filtering, or diagnostic-only churn.
- `src/pr48973-2.c` is now past the AShr fragment and failing at runtime, so do
  not treat that residual as proof that AShr lowering remains unsupported; the
  residual is a separate global-store source preservation boundary.
- AShr immediate validation is now stricter than the old generic move-to-GPR
  fallback: immediates must be `0 <= shift < width`.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step2_dumps && printf '%s\n' 'src/20020225-2.c' 'src/pr48973-2.c' > build/agent_state/395_step2_ashr.allowlist.txt && { ALLOWLIST=build/agent_state/395_step2_ashr.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step2_ashr_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/395_step2_ashr_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; probe total was 2 with `src/20020225-2.c` passing and
`src/pr48973-2.c` failing at `[RV64_BACKEND_RUNTIME_MISMATCH]`, not at
`unsupported_instruction_fragment`; the inspected `pr48973-2.c` object contains
`sraiw` for both AShr sites and exposes the later global-store value/source
clobber. Backend CTest passed all selected tests and `test_after.log` is the
canonical proof log.

Log paths:
- `build/agent_state/395_step2_ashr_probe.log`
- `build/agent_state/395_step2_pr48973_c4c_objdump.txt`
- `build/agent_state/395_step2_pr48973_c4c_bin_objdump.txt`
- `build/rv64_gcc_c_torture_backend/src_20020225-2.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr48973-2.c/case.log`
- `test_after.log`
