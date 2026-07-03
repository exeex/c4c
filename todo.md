Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement RV64 Same-Module Call Emission

# Current Packet

## Just Finished

Completed the Step 3 implementation follow-up for the Step 5
`src/20000622-1.c` blocker. The RV64 object call emitter now handles ordinary
same-module GPR call arguments whose physical source is a prepared frame-slot
home even when the `FrameSlotValue` selection still describes the original
local slot route. The repair remains semantic: it validates the selected value
id, scalar size/alignment, prepared value home, argument source slot/offset,
stack layout slot, and immediate-range offset before loading the argument.

Focused backend coverage now includes a representative-shaped same-module call
with two frame-slot GPR source arguments loaded into `a0`/`a1`, a prior-
preserved GPR source argument moved from `s2` into `a2`, and the call result
published from `a0` to the prepared owner GPR.

Reran `src/20000622-1.c` with absolute artifact/output paths:

- `src/20000412-2.c`: `rc=0`,
  `classification=lowered_and_runtime_matched`; log:
  `build/agent_state/572_rv64_same_module_call_result_lowering/src_20000412-2.c/object-route.log`.
  The representative now passes the RV64 object-route compile, link, and qemu
  runtime comparison.
- `src/20000622-1.c`: `rc=1`,
  `classification=blocked_by_later_non_call_family`; log:
  `build/agent_state/572_rv64_same_module_call_result_lowering/src_20000622-1.c/object-route.log`.
  It moved past `still_old_generic_call_fallback`.
- Exact remaining first blocker:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` for
  `/workspaces/c4c/tests/c/external/gcc_torture/src/20000622-1.c`,
  with `clang_exit=0` and `c4c_exit=Subprocess aborted`.
- Updated summary artifact:
  `build/agent_state/572_rv64_same_module_call_result_lowering/summary.tsv`.

## Suggested Next

Proceed to Step 6 close-readiness review for this runbook. The source idea's
call-fallback acceptance condition is satisfied for both representatives; the
new `src/20000622-1.c` runtime abort should be classified before any separate
follow-up idea is opened.

## Watchouts

- This plan remains limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Unsupported ordinary call ABI forms still fail closed through the existing
  generic `unsupported_instruction_fragment` surface in this bounded packet;
  precise call-specific diagnostics were not implemented here.
- The `src/20000622-1.c` rerun now reaches runtime execution and aborts, so
  any next packet should first classify the abort before expanding code changes.

## Proof

Ran:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: build succeeded; focused `backend_riscv_object_emission` CTest passed.

Proof log: `test_after.log`.

Representative rerun:

`src/20000412-2.c` was rerun through `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` with absolute `OUT_CLANG_BIN`, `OUT_OBJECT`, and `OUT_C4C_BIN` paths under `/workspaces/c4c/build/agent_state/572_rv64_same_module_call_result_lowering/src_20000412-2.c/`.

Result: `lowered_and_runtime_matched`.

`src/20000622-1.c` was rerun through `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` with absolute `OUT_CLANG_BIN`, `OUT_OBJECT`, and `OUT_C4C_BIN` paths under `/workspaces/c4c/build/agent_state/572_rv64_same_module_call_result_lowering/src_20000622-1.c/`.

Result: moved past `still_old_generic_call_fallback`; now blocked by
`[RV64_BACKEND_RUNTIME_MISMATCH]` with `c4c_exit=Subprocess aborted`.
