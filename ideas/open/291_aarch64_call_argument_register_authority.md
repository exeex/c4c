# AArch64 Call Argument Register Authority

Status: Open
Created: 2026-05-18
Split From: ideas/closed/289_aarch64_function_pointer_indirect_call_values.md

## Intent

Repair AArch64 call-boundary argument lowering so prepared register authority
for call arguments reaches the ABI registers consumed by the call.

## Why This Exists

Closure review for the function-pointer indirect-call owner separated
`tests/c/external/c-testsuite/src/00210.c` from function-pointer value loss.
The attributed function-pointer calls are preserved as indirect calls through
`actual_function`, but the remaining runtime mismatch is the `printf`
format-pointer argument move: prepared facts place `.str0` in `x21`, while the
emitted move uses `x20`.

## In Scope

- AArch64 call-boundary move lowering for pointer/scalar call arguments.
- Prepared argument register authority versus placement fallback selection.
- Focused proof beginning with `src/00210.c`.

## Out of Scope

- Function-pointer cast recovery or indirect-call callee materialization already
  covered by `ideas/closed/289_aarch64_function_pointer_indirect_call_values.md`.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, or CTest registration.
- Parser/sema attribute handling unless new evidence proves the backend call
  argument facts are correct and the frontend facts are malformed.
- Broad libc behavior and unrelated aggregate ABI work.

## Acceptance Criteria

- `src/00210.c` no longer fails from the `printf` format-pointer argument being
  moved from the wrong prepared register.
- Prepared call-argument register authority is honored when emitting AArch64
  call-boundary moves.
- Existing indirect function-pointer call shape remains indirect and valid.
- No progress is claimed through expectation, runner, allowlist, timeout, or
  unsupported-classification changes.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `src/00210.c`, `.str0`, `printf`, or one emitted move shape
  instead of repairing call-argument register authority;
- rewrites attributed function-pointer indirect calls into named direct calls;
- changes tests, expectations, runner behavior, allowlists, unsupported
  classifications, or timeout policy;
- broadens into parser/sema or libc behavior before proving the backend
  call-argument authority owner;
- renames helpers or reorganizes records while preserving the mismatch where
  prepared argument facts name one register and emission consumes another.
