# AArch64 F128 Transport Machine Printer

Status: Closed
Created: 2026-05-19
Split From: ideas/closed/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md

## Goal

Repair the AArch64 machine-printer/lowering path for structured
`f128_transport` machine nodes so focused representatives can advance past the
current printer diagnostic without weakening semantic handoff contracts.

## Why This Exists

Idea 312 closed after the local-memory semantic prepared-handoff diagnostics
were removed or bypassed for its representatives. The remaining `00204.c`
failure is now out of scope for that owner:

```text
cannot print AArch64 machine node family=f128_transport opcode=f128_transport:
f128 transport printer requires structured ...
```

The current failure occurs after semantic admission has reached AArch64
machine-node printing. This is a target machine-node spelling or lowering
owner, not a `lir_to_bir` local-memory admission owner.

## In Scope

- Identify the structured operands and value-home facts required by AArch64
  `f128_transport` machine nodes.
- Repair AArch64 lowering or machine printing so valid `f128_transport` nodes
  are selected and printable.
- Add focused backend machine-printer or dispatch coverage for the structured
  `f128_transport` shape before relying on the c-testsuite representative.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  case only after the structured backend coverage exists.

## Out Of Scope

- Reopening idea 312's semantic local-memory prepared-handoff owner.
- Reopening closed local-memory, global projection, call-boundary, or scalar
  machine-node owners by pass count alone.
- Repairing unrelated runtime, linker, assembler, timeout, direct-call, vararg,
  stack-offset, or frame-layout residuals.
- Expectation rewrites, unsupported-classification changes, allowlist changes,
  runner edits, timeout-policy changes, proof-log edits, or CTest
  registration changes.
- Filename-only, function-name-only, diagnostic-string-only, or
  c-testsuite-number-specific fixes.

## Acceptance Criteria

- Focused backend coverage proves a structured AArch64 `f128_transport` node
  prints or lowers through the intended target path.
- `c_testsuite_aarch64_backend_src_00204_c` advances past the current
  `f128_transport` printer diagnostic, or any later first-bad fact is
  classified as outside this owner.
- Existing semantic/prepared-handoff tests from idea 312 remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- adds a named-case check for `00204.c`, `stdarg`, one block number, one
  instruction number, or the exact diagnostic string instead of repairing
  structured `f128_transport` printing/lowering;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while valid `f128_transport` machine nodes still fail behind
  an equivalent unstructured-printer diagnostic;
- folds stack-offset assembly validation, runtime mismatch, timeout, direct
  call, vararg, or unrelated semantic residuals into this owner without a
  separate focused source idea;
- rewrites broad AArch64 codegen or semantic admission paths without focused
  evidence that the `f128_transport` machine-node contract requires it.

## Closure Note

Closed 2026-05-19 as complete for the structured AArch64 `f128_transport`
machine-printer/lowering owner.

Focused proof covered:

- `backend_aarch64_machine_printer`
- `backend_aarch64_instruction_dispatch`
- `backend_aarch64_target_instruction_records`
- `backend_lir_to_bir_notes`
- the existing `00204.c` semantic/prepared handoff dump guardrails
- `c_testsuite_aarch64_backend_src_00204_c`

The old `f128_transport` selected-but-unprintable diagnostic is gone. The
representative `00204.c` now reaches an out-of-scope scalar stack-publication
large-offset residual:

```text
cannot print AArch64 machine node family=scalar opcode=add:
scalar ALU stack publication offset is not printable
```

That residual is parked under
`ideas/open/314_aarch64_large_stack_offset_addressing.md`, alongside the
existing `00216.c` large stack-slot offset assembly-validation residual.

Close-time regression guard over matching focused `test_before.log` /
`test_after.log` passed in non-decreasing mode with 9 passed and 1 failed
before, 9 passed and 1 failed after, and no new failing tests.
