# AArch64 Scalar Call Value Semantics

Status: Closed
Created: 2026-05-18
Origin: Follow-on from ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Closed: 2026-05-18

## Intent

Repair AArch64 backend scalar call argument and return-value lowering for
ordinary direct calls so caller and callee agree on the AAPCS64 register
contract.

## Why This Exists

The non-leaf LR preservation route removed the old timeout owner from the
cleanest call probes. After that repair, `00100.c` and `00121.c` pass, but
`00116.c` now exits quickly with `RUNTIME_NONZERO` instead of timing out.
`00159.c` is also a quick mismatch and mixes call return values with ordinary
calls.

Those cases point at a separate semantic owner: scalar call values must be
marshaled through the AArch64 call ABI, not inferred from c-testsuite filenames
or fixed by changing runtime expectations.

## In Scope

- Inspect scalar direct-call lowering for integer arguments and integer return
  values in the AArch64 backend.
- Prove caller-side argument placement and caller-side use of returned scalar
  values with focused backend tests.
- Prove callee-side scalar parameter intake and scalar return placement when
  the existing backend test layers can observe it.
- Repair the AArch64 lowering path through existing MIR/LIR/machine-node and
  frame/call abstractions.
- Use `00116.c` and `00159.c` as runtime probes only after focused backend
  coverage identifies the semantic call-value contract.

## Out of Scope

- Variadic-call and `printf` format setup.
- String-literal addressing and external libc call conventions beyond ordinary
  scalar argument placement.
- Loop predicate, short-circuit, aggregate, pointer, static-storage, goto, and
  macro/preprocessor-adjacent failures.
- Weakening c-testsuite expected outputs, unsupported classifications,
  allowlists, CTest registration, timeout values, or runner behavior.
- Special-casing exact c-testsuite files, function names, source shapes, or
  emitted assembly snippets.

## Acceptance Criteria

- Focused AArch64 backend tests prove ordinary direct calls place scalar
  arguments and consume scalar return values according to AAPCS64.
- The repair is expressed through reusable backend call/return lowering facts,
  not named testcase handling.
- `00116.c` no longer fails because scalar call arguments or scalar call
  return values are lowered incorrectly. Any remaining failure has a different,
  recorded semantic owner.
- `00159.c` is inspected as a nearby mixed-call probe and either improves under
  the same semantic repair or is recorded as blocked by a separate owner such
  as `printf`/string-literal behavior.

## Completion Note

The active runbook completed all five steps. The implementation and proof
slices materialized AArch64 scalar call immediates, proved the focused backend
call-value path, confirmed runtime probes `00116.c` and `00159.c`, and
classified the nearby 23-case call-boundary subset with all selected cases
passing and no remaining scalar call-value owner visible in that subset.

No follow-on idea was created from this route. This closure does not claim
unrelated `printf`, string-literal addressing, variadic-call, loop,
aggregate/pointer, static-storage, short-circuit, goto, or
macro/preprocessor-adjacent families outside the proven subset.

## Reviewer Reject Signals

Reject the route if it:

- adds filename, function-name, or source-shape checks for `00116.c`,
  `00159.c`, or any c-testsuite-only pattern;
- claims capability progress through expected-output, allowlist, unsupported
  classification, CTest registration, timeout, or runner changes;
- handles only one literal argument count or return constant while leaving the
  general scalar AAPCS64 call-value path untested;
- broadens into variadic `printf`, string literals, loops, aggregates,
  short-circuit control flow, statics, or goto before ordinary scalar call
  values are proven;
- renames helpers or moves code while preserving the old caller/callee scalar
  value mismatch.
