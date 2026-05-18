Status: Active
Source Idea Path: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Review Residual Scope

# Current Packet

## Just Finished

Step 4 plan-owner residual-scope review split the broad-scan failures exposed
by Step 3 into distinct follow-up source ideas instead of expanding idea 278:

- `ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
  owns the `00004.c`/`00005.c` address-exposed local/pointer runtime-nonzero
  family.
- `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md` owns the
  `00006.c` loop/branch control runtime hang family.

Idea 278 source scope appears satisfied by the committed local materialization
repair: the focused backend subset passes, and the owned runtime smoke cases
`00001.c`, `00002.c`, and `00003.c` pass through the AArch64 backend route.
The later broad-scan failures are not the same immediate scalar local
store/load materialization owner.

Closure is not accepted in this lifecycle packet because the existing
canonical proof logs are not a valid close-gate pair. `test_before.log`
contains the focused backend subset plus the focused `00001.c`/`00002.c`/
`00003.c` route, while `test_after.log` contains the focused backend subset
plus a different 220-test broad scan that stops at `00006.c`. The regression
guard script exits PASS only because it parses different final CTest summaries
from mismatched scopes, so the pair is invalid under the regression-guard
scope rule.

Changed files:

- `todo.md`
- `ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
- `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`

Observed broad-scan results before the proof was stopped:

- 9/9 focused backend tests passed:
  `backend_aarch64_register_vocabulary`,
  `backend_aarch64_prepared_register_conversion`,
  `backend_aarch64_prepared_scalar_alu_records`,
  `backend_aarch64_prepared_scalar_cast_records`,
  `backend_aarch64_operand_resolution`,
  `backend_aarch64_return_lowering`,
  `backend_aarch64_memory_operand_records`,
  `backend_aarch64_prepared_memory_operand_records`, and
  `backend_aarch64_memory_operand_contract`.
- `c_testsuite_aarch64_backend_src_00001_c`,
  `c_testsuite_aarch64_backend_src_00002_c`, and
  `c_testsuite_aarch64_backend_src_00003_c` passed.
- `c_testsuite_aarch64_backend_src_00004_c` failed
  `[RUNTIME_NONZERO] exit=1`.
- `c_testsuite_aarch64_backend_src_00005_c` failed
  `[RUNTIME_NONZERO] exit=1`.
- `c_testsuite_aarch64_backend_src_00006_c` hung at runtime; its generated
  assembly contains an unconditional branch back to the loop label:

```asm
main:
    movz w9, #50
    str w9, [sp]
    b .LBB1_2
.LBB1_2:
    ldr w13, [sp]
    ldr w13, [sp]
    sub w19, w13, #1
    str w19, [sp]
    b .LBB1_2
    ldr w13, [sp]
    ret
```

The Step 2 local-materialization repair remains proven for the owned case. The
next owner layers are broader than the completed scalar local immediate
store/load slice: `00004.c`/`00005.c` point at address-exposed local/pointer
semantics, and `00006.c` points at branch/loop control lowering that emits an
unconditional self-loop.

## Suggested Next

Provide or regenerate matching-scope close-gate logs for idea 278, then rerun
plan-owner closure. If closure is accepted, activate
`ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
first because it is the first broad-scan blocker after the repaired
`00001.c`/`00002.c`/`00003.c` path. Keep
`ideas/open/282_aarch64_loop_branch_control_runtime_hang.md` open as the
separate `00006.c` follow-up.

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00004|00005|00006)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or CTest contracts.
- Do not add filename-specific handling for `00004.c`, `00005.c`, or
  `00006.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Preserve the idea 277 result-register repair: returned integer expression
  values must remain in `w0`/`x0` before `ret`.
- Preserve the Step 2 local immediate store/load materialization and reserved
  MIR scratch materialization (`w9`/`x9` with `movz`/`movk`).
- `00006.c` currently hangs under native runtime execution; use a timeout or
  a focused assembly-only inspection when localizing that owner to avoid
  leaving live test processes.

## Proof

Ran the delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_register_vocabulary|backend_aarch64_prepared_register_conversion|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result: blocked/incomplete. The focused backend subset passed, and the broad
scan passed `00001.c`, `00002.c`, and `00003.c`; `00004.c` and `00005.c`
failed `[RUNTIME_NONZERO] exit=1`; `00006.c` hung at runtime for several
minutes and the proof process was stopped manually to avoid leaving a live test
session. This is not `[RUNTIME_UNAVAILABLE]` and is not pass evidence. Proof
log: `test_after.log`.

Close-gate check attempted without touching proof logs:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: script exited PASS, but the logs are rejected for closure because they
are different scopes. `test_before.log` ends with the focused 3-test
`00001.c`/`00002.c`/`00003.c` route; `test_after.log` ends in the 220-test
broad scan stopped at `00006.c`. No proof logs were modified in this
plan-owner packet.
