Status: Active
Source Idea Path: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Scalar And Floating Edge Shapes

# Current Packet

## Just Finished

Step 1 - Classify Scalar And Floating Edge Shapes completed without lowering
changes.

Exact object-route diagnostics from `test_after.log`:

- `src/20000313-1.c`: `[RV64_C4C_OBJ_COMPILE_FAIL]`; prepared module shape is
  `unsupported_scalar_compare_trunc: RV64 object route supports only prepared
  named Sge i32 compare results feeding one i16 integer trunc publication`.
- `src/20020225-2.c`: `[RV64_C4C_OBJ_COMPILE_FAIL]`; prepared module shape is
  `unsupported_floating_cast: RV64 object route supports only prepared
  F32-to-F64 and F64-to-F32 FPR register casts`.
- `src/20000403-1.c`: `[RV64_C4C_OBJ_COMPILE_FAIL]`; prepared module shape is
  `unsupported_scalar_compare_trunc: RV64 object route supports only prepared
  named Sge i32 compare results feeding one i16 integer trunc publication`.

Prepared/object-route shape evidence:

- `src/20000313-1.c` / `buggy`: prepared BIR has `%t4 = bir.ugt i32 %t2, %t3`
  followed by `%t6 = bir.sub i32 0, %t4`; `%t4` is stack-homed and `%t6` is a
  GPR value stored to local `%lv.borrow`, then returned. This is a non-branch
  unsigned i32 compare-result publication consumed by integer arithmetic, not
  the currently admitted named `Sge i32` plus `i16` trunc shape.
- `src/20000403-1.c` / `seqgt`: prepared BIR has `%t5 = bir.sgt i64 %t2, 0`
  returned as `i32`; operands and result are GPR homes. This is a non-branch
  signed i64 compare-result publication to the integer return path, not an
  `i16` trunc publication.
- `src/20000403-1.c` / `seqgt2`: prepared BIR has `%t6 = bir.sgt i64 %t3, 0`
  after an i64 local load, returned as `i32`; operands and result are GPR
  homes. This is the same scalar compare-result publication family with a
  local-load producer.
- `src/20020225-2.c` / `test`: prepared BIR has
  `%t1 = bir.sitofp i32 0 to double`, stored to union storage; `%t1` has FPR
  home `ft0` and the store-source publication identifies a cast producer. This
  is an integer-to-F64 cast, not the existing F32-to-F64 or F64-to-F32 FPR
  register cast helper shape.

## Suggested Next

First implementation packet should target scalar compare-result publication,
not floating cast and not a producer split. The scalar bucket has complete
prepared facts for compare operands, value homes, and result consumers, and it
covers two representative cases here. The floating case is separable
integer-to-F64 conversion work.

Suggested narrow proof command for that scalar packet:

```bash
cmake --build --preset default --target c4cll && mkdir -p build/agent_state && printf '%s\n' 'src/20000313-1.c' 'src/20000403-1.c' 'src/int-compare.c' 'src/compare-2.c' 'src/pr48973-2.c' > build/agent_state/401_scalar_compare_trunc.allowlist.txt && { ALLOWLIST=build/agent_state/401_scalar_compare_trunc.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_scalar_compare_trunc|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log
```

## Watchouts

- `ideas/closed/403_prepared_i16_formal_abi_publication.md` is complete; do not
  reopen `I16` formal ABI publication in this object-route plan.
- Do not special-case `src/20000313-1.c`, `src/20020225-2.c`,
  `src/20000403-1.c`, function names, or constants.
- Split missing prepared producer facts into a separate idea instead of
  compensating inside RV64 object emission.
- The current scalar diagnostic name mentions truncation, but the classified
  representatives are broader non-branch compare-result publications:
  compare-to-arithmetic and compare-to-return. Do not clear the diagnostic by
  only extending the old `Sge i32`/`i16` trunc pattern.
- The floating representative is `SIToFP i32 0 -> double`; the existing helper
  only admits FPR-register `FPExt`/`FPTrunc`. Keep that as a later floating
  packet unless scalar lowering exposes a shared helper need.

## Proof

Ran the supervisor-selected Step 1 proof exactly:

```bash
cmake --build --preset default --target c4cll && mkdir -p build/agent_state && printf '%s\n' 'src/20000313-1.c' 'src/20020225-2.c' 'src/20000403-1.c' > build/agent_state/401_step1_edges.allowlist.txt && { ALLOWLIST=build/agent_state/401_step1_edges.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_scalar_compare_trunc|unsupported_floating_cast|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log
```

Result: `test_after.log` reports total=3, passed=0, failed=3, with the exact
diagnostics recorded above. Prepared BIR classification dumps used for this
packet are under `build/agent_state/401_step1_dumps/`.
