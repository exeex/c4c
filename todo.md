Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Broader AArch64 Backend Runtime Inventory

# Current Packet

## Just Finished

Step 2 started the broader AArch64 backend c-testsuite inventory. CTest
selected 220 `aarch64_backend` cases. The first six cases,
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c`,
passed through the real backend runtime route. The next selected case,
`c_testsuite_aarch64_backend_src_00007_c`, reached `.s` generation and
`clang -x assembler` executable generation, then hung at runtime. I stopped the
run after roughly 2.5 minutes with no CTest progress and recorded the manual
stop in `test_after.log`.

Route-stage inventory from this partial broad scan:

- selected: 220
- passed: 6
- `[RUNTIME_HANG]`: 1 (`00007.c`)
- not run after stop: 213
- `[FRONTEND_FAIL]`, `[BACKEND_OUTPUT_MISSING]`, `[BACKEND_FALLBACK_IR]`,
  `[BACKEND_FAIL]`, `[RUNTIME_UNAVAILABLE]`, `[RUNTIME_NONZERO]`,
  `[RUNTIME_MISMATCH]`: 0 observed before the hang

The first distinct owner layer is AArch64 backend loop/control lowering for
`00007.c`, not the runtime route itself. Generated artifacts exist:
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00007.c.s` and
`00007.c.bin`. The assembly shows an unconditional self-loop before the
expected loop-exit condition:

```asm
.LBB1_2:
ldr w13, [sp]
ldr w13, [sp]
sub w19, w13, #1
str w19, [sp]
b .LBB1_2
```

The same file later emits another unconditional loop header:

```asm
.LBB1_8:
ldr w13, [sp]
b .LBB1_8
```

This is hang evidence, not pass evidence. The route up to the first blocker is
still real AArch64 backend assembly; no LLVM IR fallback, expectation
weakening, allowlist change, unsupported reclassification, or runner edit was
used.

## Suggested Next

Open or activate a focused follow-up idea for the `00007.c` loop-control
runtime hang. The smallest next packet should localize why the AArch64 backend
emits unconditional self-loops for the `for` loop control shape in `00007.c`
after the `00006.c` fused-compare loop case was repaired.

## Watchouts

- Do not fold backend semantic repairs directly into idea 276; split remaining
  capability gaps into focused follow-up ideas.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Preserve distinct route-stage classifications. The broad scan is currently
  blocked at `[RUNTIME_HANG]` for `00007.c`, with only six broad-route passes
  recorded before the stop.
- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, runner files, or timeout policy to claim route progress.
- `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md` remains
  open but inactive; do not reactivate it unless supervisor chooses to close or
  repair that lifecycle record separately.

## Proof

Step 2 proof command:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result: BLOCKED by runtime hang. The command selected 220 tests and passed
`00001.c` through `00006.c`, then hung while running
`c_testsuite_aarch64_backend_src_00007_c`. I killed the `00007.c.bin` runtime,
runner, CTest, and wrapper processes after roughly 2.5 minutes with no CTest
progress. `test_after.log` contains the partial output plus a `[MANUAL_STOP]`
line documenting the stop.
