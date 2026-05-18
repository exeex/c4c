# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Minimal Local Function-Pointer Failure

# Current Packet

## Just Finished

Closed the AArch64 stack-frame/SP alignment idea and switched to the focused
function-pointer indirect-call value idea.

Source evidence from the SP-alignment closure:
- `src/00004.c` passed after the frame-alignment repair.
- Nearby local pointer/array subset `00004/00005/00013/00014/00015/00016`
  passed 6/6.
- Wider former bus-error subset selected 28/28: 17 passed, 11 failed, 0 timed
  out.
- Remaining bus-error cases `src/00087.c`, `src/00089.c`, `src/00124.c`, and
  `src/00210.c` have aligned frames (`sub sp, sp, #32` or `#48`) and a bad or
  missing function-pointer value before indirect `blr`.

## Suggested Next

Supervisor should delegate Step 1 to establish the minimal `src/00087.c`
function-pointer value failure shape, inspect function symbol materialization,
the local aggregate field store/load, and the indirect `blr` callee register,
then capture the baseline proof/log for this owner.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact function-name shortcuts, aggregate-field
  shortcuts, and emitted-instruction-text matching.
- Do not treat this as residual stack-frame/SP alignment unless a new
  unaligned frame is proven.
- Preserve direct-call behavior and existing aligned frame behavior.
- Keep non-bus runtime failures from the wider sample outside this route unless
  they are proven to share the same function-pointer indirect-call owner.
- Keep parser/sema, broad aggregate ABI, compare/branch lowering, libc
  behavior, and timeout-sensitive cases out of this route.

## Proof

Lifecycle close/switch only. No build or test proof was run.
