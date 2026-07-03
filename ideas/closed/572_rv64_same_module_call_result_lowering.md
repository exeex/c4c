# RV64 Same-Module Call Result Lowering

Status: Closed
Type: Capability repair
Parent: `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
Owning Layer: RV64 object lowering for ordinary call instructions and results

## Goal

Implement the RV64 object-route lowering needed for ordinary same-module BIR
`CallInst` nodes with GPR arguments and integer results, including result
publication back into the prepared value environment.

## Why This Exists

The Step 3 diagnostics from the 570 runbook identified two retained
representatives whose first object-route blocker is an ordinary `CallInst`
with a value owner:

- `src/20000412-2.c`: `function=main`, `owner=i32 %t0`, same-module call to
  `f` with immediate/null arguments and a GPR result.
- `src/20000622-1.c`: `function=baz`, `owner=i64 %t4`, same-module call to
  `foo` with three GPR arguments after a prior call result.

Evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- Per-case prepared-BIR and object-route logs under
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`

## In Scope

- RV64 object emission for ordinary same-module calls represented as BIR
  `CallInst` nodes.
- Integer and pointer-sized GPR argument materialization for the observed
  same-module call shapes.
- Integer/GPR call result capture and publication to the call instruction's
  owner value.
- Focused tests for call result publication and multiple GPR arguments.
- Diagnostics that distinguish unsupported call ABI forms from the generic
  unsupported instruction fallback.

## Out Of Scope

- `llvm.inline_asm` carrier handling.
- Floating-point argument/result ABI, varargs, aggregate returns, tail calls,
  or external linkage policy unless needed to keep unsupported forms precisely
  diagnosed.
- Select, pointer arithmetic, prepared authority, or runtime comparison work.
- Adding named special cases for the representative source files.

## Acceptance Criteria

- The two same-module call representatives no longer fail at the first
  ordinary `CallInst` solely because call result lowering is absent, or they
  fail with a narrower call-ABI diagnostic for a genuinely unsupported call
  shape.
- Focused backend tests prove GPR argument passing and integer result
  publication for same-module calls.
- Unsupported call forms continue to fail closed with useful diagnostics.
- No inline asm, select, FP binary, or pointer arithmetic work is mixed into
  the same acceptance slice.

## Closure Notes

Closed after the call-specific diagnostic follow-up resolved the previous
close blocker. Focused backend coverage now keeps unsupported ordinary
same-module call ABI/result shapes on `unsupported_call_abi` instead of the
generic `unsupported_instruction_fragment` fallback.

Step 5 representative evidence:

- `src/20000412-2.c`: lowered and runtime-matched.
- `src/20000622-1.c`: advanced past the old ordinary `CallInst` fallback and
  is now tracked separately as
  `ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md`.

Close-time regression guard used existing backend CTest logs:
`test_before.log` and `test_after.log`, both 346/346 passed, with
non-decreasing comparison accepted because the backend bucket count was
unchanged.

## Reviewer Reject Signals

- Reject a slice that treats `llvm.inline_asm` carriers as ordinary calls to
  make call tests pass.
- Reject filename-specific or exact call-signature shortcuts for
  `20000412-2.c` or `20000622-1.c`.
- Reject expectation rewrites, unsupported-marker changes, or allowlist edits
  claimed as call capability progress.
- Reject a route that stores a call result under a new abstraction name while
  leaving later users to observe the same missing-result failure.
- Reject broad ABI rewrites that do not include focused proof for ordinary
  same-module GPR calls and result publication.
