Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm Producer-Side I16 Gap

# Current Packet

## Just Finished

Step 1 - Confirm Producer-Side I16 Gap completed as classification-only
evidence gathering for `src/20000403-1.c`; no implementation files were
changed.

Evidence:

- Existing prepared dump:
  `build/agent_state/395_step4_20000403-1.prepared-bir.txt`.
- The dump shows the representative formals:
  `bir.func @seqgt(i64 %p.a, i16 %p.win, i64 %p.b) -> i32` and
  `bir.func @seqgt2(i64 %p.a, i16 %p.win, i64 %p.b) -> i32`.
- In the same dump, `%p.win` is stack-homed without a usable scalar integer
  ABI fact:
  `prepared.summary @seqgt` has
  `storage %p.win frame_slot bank=none spill_slot=slot#3+stack0 offset=0`,
  and `prepared.summary @seqgt2` has
  `storage %p.win frame_slot bank=none spill_slot=slot#5+stack8 offset=8`.
- The prepared homes agree:
  `prepared.func @seqgt` has
  `home %p.win value_id=19 kind=stack_slot slot_id=3 offset=0`, and
  `prepared.func @seqgt2` has
  `home %p.win value_id=26 kind=stack_slot slot_id=5 offset=8`.
- The prepared stack layout records both homes as regalloc spill slots with
  `type=i16 size=2 align=2`, not durable ABI homes:
  object `#3` for `seqgt` and object `#5` for `seqgt2`.
- Fresh narrow proof reproduces the object-route admission failure in
  `test_after.log`: `RV64_C4C_OBJ_COMPILE_FAIL` with
  `unsupported_param_home: RV64 object route requires all parameters in
  supported GPR or prepared FPR register homes`.
- `src/backend/prealloc/legalize.cpp` currently defines
  `direct_bir_call_arg_abi_repair()` with Integer scalar coverage for `I1`,
  `I8`, `I32`, `I64`, and `Ptr`; `I16` is absent from that dispatch.

## Suggested Next

Execute Step 2 by adding producer-side `I16` handling to
`direct_bir_call_arg_abi_repair()` through the same Integer ABI path as `I1`,
`I8`, `I32`, `I64`, and `Ptr`, then rerun the proof command below.

## Watchouts

- Keep the repair producer-side in `src/backend/prealloc/legalize.cpp`; do not
  reconstruct missing `I16` formal ABI in RV64 `object_emission.cpp`.
- Do not special-case `src/20000403-1.c`, `seqgt`, `seqgt2`, or `%p.win`.
- The expected first success signal is that `src/20000403-1.c` no longer fails
  at the same `unsupported_param_home` admission point; it may still expose a
  later, distinct RV64 object-route blocker after the producer fact is
  published.

## Proof

Ran the supervisor-delegated proof command exactly; log path: `test_after.log`.
The command rebuilt `c4cll`, wrote
`build/agent_state/403_step1_i16_formal.allowlist.txt`, and reproduced the
known one-case failure:

```sh
cmake --build --preset default --target c4cll && mkdir -p build/agent_state && printf '%s\n' 'src/20000403-1.c' > build/agent_state/403_step1_i16_formal.allowlist.txt && { ALLOWLIST=build/agent_state/403_step1_i16_formal.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_param_home|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log
```

Result excerpt: `fail	src/20000403-1.c`, `[RV64_C4C_OBJ_COMPILE_FAIL]`,
`unsupported_param_home`, and `[rv64-gcc-torture] total=1 passed=0 failed=1`.
