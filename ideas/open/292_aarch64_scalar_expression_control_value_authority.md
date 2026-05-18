# AArch64 Scalar Expression Control-Value Authority

Status: Open
Created: 2026-05-18
Split From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Intent

Repair the AArch64 backend path where scalar expression results, branch
predicates, return values, and scalar call arguments must be emitted from the
authoritative semantic value instead of stale scratch registers or placeholder
locations.

## Why This Exists

The refreshed AArch64 c-testsuite inventory after the closed AArch64 backend
owners still shows a coherent runtime failure family where scalar values are
computed, selected, or loaded in one place but later consumers use a different
stale register or unmaterialized value.

Representative evidence from Step 3:

- `src/00009.c` computes ordinary `*`, `/`, `%`, and subtraction results, but
  generated assembly reloads a local slot and stores stale `w19` instead of
  publishing each arithmetic result.
- `src/00012.c` should return constant 0, but generated assembly emits a stale
  subtraction result.
- `src/00056.c` and `src/00211.c` prepare valid format pointers but omit or
  mis-source scalar varargs.
- `src/00156.c` and `src/00161.c` lose loop predicate or induction-state
  values, causing early exit or stale loop output.

The selected owner is broader than one arithmetic opcode or one c-testsuite
file. It is the authority chain that connects scalar semantic values to the
registers consumed by arithmetic publication, stores, comparisons, returns,
branches, and scalar call arguments.

## In Scope

- AArch64 scalar expression result publication for integer constants, local
  loads, selected arithmetic results, and simple assigned values.
- Branch predicate and loop-control lowering when the predicate should consume
  the current scalar value.
- Scalar return-value and scalar call-argument materialization when the value
  is already a scalar semantic fact.
- Backend/prealloc authority plumbing that chooses between prepared semantic
  value facts and fallback scratch or callee-saved registers.
- Focused proof beginning with `src/00009.c`, `src/00012.c`, `src/00056.c`,
  `src/00156.c`, `src/00161.c`, and `src/00211.c`.

## Out of Scope

- Pointer/aggregate address authority such as `src/00019.c` or remaining
  aggregate/function-pointer edge cases.
- Timeout or hang repair for `src/00132.c`, `src/00173.c`, `src/00220.c`, or
  other loop/string/wide-character compound cases.
- Compile-stage backend printer/admission gaps such as scalar `sub`, `or`,
  `xor`, or fused compare-branch operand forms.
- Reopening closed owners for non-leaf LR preservation, scalar call-value
  semantics, string/global external-call lowering, stack-frame/SP alignment,
  function-pointer indirect-call values, scalar parameter ALU authority, or
  call-argument register authority without contradictory proof.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, CTest registration, parser/sema rewrites, libc behavior, or
  broad aggregate ABI work.

## Acceptance Criteria

- Starter representatives `src/00009.c`, `src/00012.c`, `src/00056.c`,
  `src/00156.c`, `src/00161.c`, and `src/00211.c` no longer fail from stale or
  missing scalar value publication.
- Generated AArch64 assembly shows scalar consumers using the authoritative
  semantic value for arithmetic publication, branch predicates, return values,
  and scalar call arguments instead of stale callee-saved/scratch registers.
- Nearby same-family scalar expression/control cases are sampled enough to
  show the repair is semantic rather than filename-specific.
- Pointer/aggregate, timeout, and compile-stage printer failures remain
  explicitly separated unless fresh proof shows they share the repaired scalar
  authority primitive.
- No progress is claimed through expectation, runner, allowlist, timeout, or
  unsupported-classification changes.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `src/00009.c`, `src/00012.c`, `src/00056.c`, `src/00156.c`,
  `src/00161.c`, `src/00211.c`, one local variable name, one loop, or one
  emitted instruction sequence instead of repairing scalar value authority;
- changes expected outputs, runner behavior, allowlists, unsupported
  classifications, timeout policy, or CTest registration;
- claims capability progress through helper renames, classification rewrites,
  or expectation churn while scalar consumers still read stale `w19`/`x19`,
  stale scratch registers, or uninitialized call-argument values;
- broadens into pointer/aggregate address repair, timeout-loop investigation,
  compile-stage printer gaps, parser/sema, libc, or aggregate ABI work before
  proving the scalar expression/control owner;
- reopens a closed AArch64 owner from residual failing counts alone without
  generated-code evidence contradicting that owner's closure;
- preserves the old failure mode behind a new abstraction name where prepared
  scalar facts exist but emission still consumes fallback registers.
