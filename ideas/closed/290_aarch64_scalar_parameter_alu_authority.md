# AArch64 Scalar Parameter ALU Authority

Status: Closed
Created: 2026-05-18
Split From: ideas/closed/289_aarch64_function_pointer_indirect_call_values.md

## Intent

Repair the AArch64 native backend path where scalar function parameters should
arrive in the registers consumed by selected scalar ALU operations.

## Why This Exists

Closure review for the function-pointer indirect-call owner separated
`tests/c/external/c-testsuite/src/00124.c` from function-pointer value loss.
The generated code already returns the selected function pointer and invokes it
through `blr`, but the called function computes with stale callee-saved
registers instead of incoming `w0`/`w1` parameters.

## In Scope

- AArch64 scalar parameter home publication and selected ALU operand authority.
- Call entry parameter register facts that must feed scalar operations.
- Focused proof beginning with `src/00124.c`.

## Out of Scope

- Function-pointer value materialization or indirect-call callee setup already
  covered by `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, or CTest registration.
- Parser, sema, broad aggregate ABI, libc behavior, and unrelated call-chain
  repair.

## Acceptance Criteria

- `src/00124.c` no longer fails from stale scalar parameter values in the
  returned function-pointer call chain.
- Generated AArch64 scalar ALU inputs for function parameters are backed by
  authoritative incoming parameter registers.
- No progress is claimed through expectation, runner, allowlist, timeout, or
  unsupported-classification changes.

## Closure Summary

Closed on 2026-05-18 after Step 4 owner-boundary validation.

The scalar parameter/ALU owner is complete:

- The focused AArch64 scalar ALU backend test passes.
- `src/00124.c` passes.
- Generated `f2` consumes authoritative incoming scalar parameter registers:

```asm
sub w0, w0, w1
ret
```

The function-pointer return and indirect-call path remained valid and outside
this owner.

Close validation used the matching `src/00124.c` root proof logs without
modifying them:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed with no new failures.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `src/00124.c`, one function name, or one arithmetic expression
  instead of repairing parameter-to-ALU authority;
- rewrites the function-pointer indirect call into a direct call to hide the
  scalar parameter defect;
- changes tests, expectations, runner behavior, allowlists, unsupported
  classifications, or timeout policy;
- broadens into aggregate ABI, parser/sema, libc, or unrelated call-lowering
  work before proving the scalar parameter owner;
- renames helpers or reorganizes records while leaving selected ALU operations
  to consume stale callee-saved registers.
