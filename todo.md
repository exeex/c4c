Status: Active
Source Idea Path: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Generalize Scalar Compare/Trunc Lowering

# Current Packet

## Just Finished

Step 2 - Generalize Scalar Compare/Trunc Lowering completed for the classified
non-branch scalar compare-result publication forms.

Implemented semantic RV64 object-route compare value materialization in
`fragment_for_prepared_binary` for `slt`, `sle`, `sgt`, `sge`, `ult`, `ule`,
`ugt`, and `uge`, reusing existing prepared value movement and destination
home/stack publication handling. The old `Sge i32` compare-to-`i16` trunc
helper remains an accepted specialized path, but it is no longer the admission
gate for scalar compare publication diagnostics.

Before/after diagnostic movement in the delegated probe:

- `src/20000313-1.c`: moved from `[RV64_C4C_OBJ_COMPILE_FAIL]`
  `unsupported_scalar_compare_trunc` to `pass`.
- `src/20000403-1.c`: moved from `[RV64_C4C_OBJ_COMPILE_FAIL]`
  `unsupported_scalar_compare_trunc` to `pass`.
- `src/int-compare.c`: still fails, now with generic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `src/compare-2.c`: still fails, now with generic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `src/pr48973-2.c`: still fails, now with generic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

Focused backend coverage updated
`backend_riscv_object_emission_test.cpp` with a direct ordered compare-return
object build and with a scalar compare publication failure boundary that is now
a real missing prepared result home instead of formerly unsupported compare
opcodes, i64 operands, or stack-homed compare results.

## Suggested Next

Next coherent packet should classify the three remaining generic
`unsupported_instruction_fragment` failures in the Step 2 allowlist
(`src/int-compare.c`, `src/compare-2.c`, `src/pr48973-2.c`) into specific BIR
instruction/helper boundaries before implementing another lowering change.

## Watchouts

- `ideas/closed/403_prepared_i16_formal_abi_publication.md` is complete; do not
  reopen `I16` formal ABI publication in this object-route plan.
- Do not special-case `src/20000313-1.c`, `src/20000403-1.c`,
  `src/int-compare.c`, `src/compare-2.c`, `src/pr48973-2.c`, function names, or
  constants.
- Split missing prepared producer facts into a separate idea instead of
  compensating inside RV64 object emission.
- The remaining generic diagnostics are not scalar compare-result publication
  blockers anymore; the representative scalar cases pass without producer fact
  reconstruction.
- The Step 1 floating representative remains separable `SIToFP i32 0 ->
  double` work and was intentionally not included in this scalar packet.

## Proof

Ran the supervisor-selected Step 2 proof exactly:

```bash
cmake --build --preset default && mkdir -p build/agent_state && printf '%s\n' 'src/20000313-1.c' 'src/20000403-1.c' 'src/int-compare.c' 'src/compare-2.c' 'src/pr48973-2.c' > build/agent_state/401_scalar_compare_trunc.allowlist.txt && { ALLOWLIST=build/agent_state/401_scalar_compare_trunc.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/401_scalar_compare_trunc_probe.log 2>&1 || true; } && rg -n 'unsupported_scalar_compare_trunc|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' build/agent_state/401_scalar_compare_trunc_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; probe log
`build/agent_state/401_scalar_compare_trunc_probe.log` reports total=5,
passed=2, failed=3; backend CTest passed and is recorded in `test_after.log`.
