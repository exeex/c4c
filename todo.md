Status: Active
Source Idea Path: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Pointer and String Mutation Failures

# Current Packet

## Just Finished

Step 3 repaired AArch64 pointer-local reload publication for wide-string local
array traversal. `00220` now reloads the pointer local from its frame slot before
the loop latch increment and before dereferencing after calls, instead of
trusting stale caller-saved register state after `printf`.

Files changed:
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`

Before/after subset movement:
- PASS `c_testsuite_aarch64_backend_src_00220_c`; moved from segmentation fault
  to passing. Generated latch now reloads `%lv.p` with `ldr x13, [sp, #88]`
  before `add x9, x13, #4`.
- PASS `c_testsuite_aarch64_backend_src_00172_c`; moved to passing in this
  proof window before this packet's owned change was accepted.
- PASS `c_testsuite_aarch64_backend_src_00180_c`; remained passing.
- PASS `c_testsuite_aarch64_backend_src_00216_c`; remained passing after
  excluding stack-homed aggregate pointer-copy results from the new reload
  helper.
- PASS `c_testsuite_aarch64_backend_src_00204_c`; remained passing after
  leaving AAPCS64 variadic `%lv.ap.*` local slots on the existing va_list
  materialization path.
- FAIL `backend_aarch64_instruction_dispatch`; unchanged f64 call-ABI selected
  value publication expectation.

## Suggested Next

Continue Step 3 with the remaining `backend_aarch64_instruction_dispatch` f64
selected call-ABI publication failure, or route `00172` according to the
supervisor's next proving window if it reappears outside this subset.

## Watchouts

The reload helper is deliberately narrow: frame-slot loads from pointer
`local_slot` objects into register-like homes only, excluding stack-homed
aggregate pointer-copy results and AAPCS64 variadic `%lv.ap.*` bookkeeping
slots. Do not broaden it without rechecking `00216` and `00204`.
Avoid expectation rewrites.

## Proof

Ran:
`cmake --build --preset default; ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1`

Build completed after rebuilding the AArch64 backend objects touched by this
packet.
Final CTest result after this packet: 5/6 passed, 1/6 failed. Passing targets:
`00172`, `00180`, `00216`, `00220`, `00204`. Failing target:
`backend_aarch64_instruction_dispatch`. Proof log: `test_after.log`.
