Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Review And Close Readiness

# Current Packet

## Just Finished

Step 6 close-readiness review rejected closure for the 572 source idea.

Accepted as satisfied:

- The old ordinary same-module `CallInst` fallback is gone for both retained
  representatives. `build/agent_state/572_rv64_same_module_call_result_lowering/summary.tsv`
  records `src/20000412-2.c` as `lowered_and_runtime_matched` and
  `src/20000622-1.c` as `blocked_by_later_non_call_family`.
- The `src/20000622-1.c` remaining failure is not a call-fallback failure: the
  rerun now reaches runtime comparison and aborts with
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
  `c4c_exit=Subprocess aborted`.
- Focused backend coverage includes same-module GPR argument passing, prior
  call-result consumption, frame-slot GPR arguments, and integer/GPR result
  publication.

Close blocker:

- The source idea still has an unmet diagnostic acceptance item. It explicitly
  scopes in diagnostics that distinguish unsupported call ABI forms from the
  generic unsupported instruction fallback, but the focused unsupported
  same-module call shapes still assert
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
  This does fail closed, but it is not the narrower call-specific diagnostic
  required by the active source idea and Step 6 runbook.

Lifecycle split:

- Created `ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md`
  for the distinct `src/20000622-1.c` runtime abort. Do not solve that abort
  inside this 572 plan unless the supervisor explicitly switches lifecycle
  state.

## Suggested Next

Keep 572 active and delegate a narrow executor packet for the remaining
diagnostic acceptance item: unsupported ordinary same-module call ABI/result
shapes should reject with a call-specific diagnostic rather than the generic
`unsupported_instruction_fragment` fallback. After that passes focused backend
proof, rerun Step 6 close-readiness and then the close-time backend regression
gate.

## Watchouts

- This plan remains limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Do not use the new `src/20000622-1.c` runtime-abort follow-up as a reason to
  expand 572. The abort is separate from the ordinary same-module call fallback
  repair.
- Do not close 572 until unsupported ordinary same-module call ABI/result
  forms have distinguishable call-specific diagnostics or the source idea is
  explicitly narrowed by the supervisor.

## Proof

Close-readiness inputs reviewed:

- Source idea: `ideas/open/572_rv64_same_module_call_result_lowering.md`.
- Active runbook: `plan.md`.
- Step 5 summary:
  `build/agent_state/572_rv64_same_module_call_result_lowering/summary.tsv`.
- Step 5 `src/20000622-1.c` log:
  `build/agent_state/572_rv64_same_module_call_result_lowering/src_20000622-1.c/object-route.log`.
- Existing broad backend proof in `test_before.log`: 346/346 backend tests
  passed.

The close-time regression gate was not run because source-idea completion is
false.
