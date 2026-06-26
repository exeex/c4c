Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Refresh Bucket Counts And Route The Next Family

# Current Packet

## Just Finished

Step 3 refreshed the RV64 gcc-torture backend bucket after the scalar AShr
lowering packet and classified the next 395-scope instruction-fragment family.

Refreshed bucket:
- Full probe total: 1467 cases, 337 passed, 1130 failed.
- Current diagnostic counts in the probe log: 295
  `unsupported_instruction_fragment`, 40 `unsupported_terminator_fragment`, 83
  `unsupported_stack_frame`, 30 `unsupported_local_memory_access`, 34
  `unsupported_global_data`, 64 `[RV64_BACKEND_RUNTIME_MISMATCH]`, and 444
  `backend object route requires semantic` failures.

AShr family status:
- `src/20000223-1.c` passes in the refreshed full probe.
- `src/20020225-2.c` passes in the refreshed full probe.
- `src/pr48973-2.c` no longer fails with
  `unsupported_instruction_fragment`; it now fails at
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, matching the prior global-store
  source/value clobber residual. Route that residual outside idea 395, most
  likely to idea 402 runtime-mismatch ownership.

Next family evidence:
- Scanned the 295 current `unsupported_instruction_fragment` cases and dumped
  BIR/prepared BIR artifacts under `build/agent_state/395_step3_dumps/`.
- The largest obvious still-unlowered scalar arithmetic family is signed
  division/remainder: `bir.sdiv` appears 232 times and `bir.srem` appears 166
  times across the remaining instruction-fragment cases; related unsigned
  remainder appears 30 times.
- Concrete representative: `src/divcmp-1.c` fails with
  `unsupported_instruction_fragment`; prepared BIR starts `test1` with
  `%t0 = bir.sdiv i32 %p.x, 10`, where `%p.x` and `%t0` both have prepared GPR
  homes.
- Nearby same-family case: `src/divmod-1.c` fails with the same diagnostic and
  contains register-homed `bir.sdiv i32`, `bir.srem i32`, `bir.srem i64`, and
  `bir.urem i64` sites.

## Suggested Next

Delegate Step 4 to implement the next 395-scope prepared scalar division
family in `fragment_for_prepared_binary()`, starting with `BinaryOpcode::SDiv`
for I32/I64 GPR/rematerializable operands and using `src/divcmp-1.c` as the
representative plus `src/divmod-1.c` as the nearby same-family case. If the
packet includes remainder, keep `SRem` and `URem` tied to the same semantic
division/remainder family; otherwise split them into the immediately following
packet.

## Watchouts

- Do not absorb stack-frame or parameter-home work owned by idea 398; current
  probe still has 83 `unsupported_stack_frame` cases.
- Do not absorb runtime mismatches owned outside this idea; current probe has
  64 runtime mismatches, including `src/pr48973-2.c`.
- Do not infer missing producer facts inside RV64 object emission.
- Do not claim progress from filename-specific lowering, expectation rewrites,
  allowlist filtering, or diagnostic-only churn.
- `src/pr48973-2.c` is now past the AShr fragment and failing at runtime, so do
  not treat that residual as proof that AShr lowering remains unsupported; the
  residual is a separate global-store source preservation boundary.
- AShr immediate validation is now stricter than the old generic move-to-GPR
  fallback: immediates must be `0 <= shift < width`.
- Some early remaining instruction-fragment cases, such as `src/20071211-1.c`,
  expose inline-asm/global-bitfield shapes before the division-heavy family;
  keep those classified separately if the next implementation packet proves
  they are not scalar binary arithmetic lowering.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step3_dumps && { STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step3_instruction_bucket_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/395_step3_instruction_bucket_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; full probe completed with `total=1467 passed=337
failed=1130`; `src/20000223-1.c` and `src/20020225-2.c` passed; `src/pr48973-2.c`
failed at `[RV64_BACKEND_RUNTIME_MISMATCH]`, not
`unsupported_instruction_fragment`. Backend CTest passed all selected
`^backend_` tests and `test_after.log` is the canonical proof log.

Log paths:
- `build/agent_state/395_step3_instruction_bucket_probe.log`
- `build/agent_state/395_step3_unsupported_instruction_cases.txt`
- `build/agent_state/395_step3_opcode_scan.tsv`
- `build/agent_state/395_step3_dumps/divcmp-1.prepared.txt`
- `build/agent_state/395_step3_dumps/divmod-1.prepared.txt`
- `build/rv64_gcc_c_torture_backend/src_20020225-2.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr48973-2.c/case.log`
- `test_after.log`
