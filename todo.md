# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Review

# Current Packet

## Just Finished

Executed Step 5 closure review for the active function-pointer source idea.

The focused four-case owner subset now separates cleanly:

- `src/00087.c` passes for the local function-pointer value/callee repair.
- `src/00089.c` passes for the global/static function-pointer initializer
  repair.
- `src/00124.c` remains outside this owner on the already separated scalar
  parameter/ALU blocker.
- `src/00210.c` remains outside this owner on the already separated AArch64
  call-argument/register authority blocker.

No test, runner, expected-output, allowlist, unsupported-classification, or
timeout changes were involved.

This source idea appears ready for a plan-owner close decision for the
function-pointer owner. The two remaining focused-subset failures are not
function-pointer value/callee loss:

- `00124` already returns the selected function pointer and calls through it:

```asm
f1:
    adrp x0, f2
    add x0, x0, :lo12:f2
    ret
main:
    blr x13
```

- `00210` already preserves both attributed function-pointer cast calls as
  indirect calls through `actual_function`:

```asm
adrp x20, actual_function
add x20, x20, :lo12:actual_function
blr x20
```

The remaining `00210` runtime mismatch is the `printf` format-pointer argument
move, not the function-pointer call path:

```asm
adrp x21, .str0
add x21, x21, :lo12:.str0
mov x0, x20
mov x1, x13
bl printf
```

## Suggested Next

Hand off to the plan owner for the close/deactivate/split decision. If the
source idea is closed, keep separate follow-up ownership for the `00124` scalar
parameter/ALU blocker and the `00210` AArch64 call-argument/register authority
blocker.

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
- `src/00087.c` passes in the closure subset.
- `src/00089.c` remains accepted for the Step 3 function-pointer owner.
- `src/00124.c` remains separated on the scalar parameter/ALU blocker:
  generated `f1` returns `f2` via `x0`, and `main` performs `blr x13`, but
  generated `f2` still computes with stale callee-saved registers instead of
  incoming `w0`/`w1` parameters.
- `src/00210.c` is blocked outside the function-pointer owner by a general
  AArch64 call-argument register-authority mismatch for the `printf` format
  string: prepared facts say `.str0` is in `x21`, but the emitted move uses
  `x20`.
- Do not convert indirect calls to named direct calls; the repaired shape still
  uses `blr`.

## Proof

Ran the delegated Step 5 closure-review proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00(087|089)_c$|^c_testsuite_aarch64_backend_src_00124_c$|^c_testsuite_aarch64_backend_src_00210_c$'; } > test_after.log 2>&1
```

Result: selected 4 tests. `c_testsuite_aarch64_backend_src_00087_c` passed,
and `c_testsuite_aarch64_backend_src_00089_c` passed.
`c_testsuite_aarch64_backend_src_00124_c` failed `[RUNTIME_NONZERO] exit=200`
from the separated scalar parameter/ALU blocker.
`c_testsuite_aarch64_backend_src_00210_c` failed `[RUNTIME_MISMATCH]` from the
separated `printf` call-argument/register authority blocker. `test_after.log`
is the canonical proof log for this Step 5 packet.
