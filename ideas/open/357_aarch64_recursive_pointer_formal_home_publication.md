# AArch64 Recursive Pointer Formal Home Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md

## Goal

Repair same-module recursive call paths where pointer formals assigned to
callee-saved homes must be published before recursive calls and helper calls
consume them.

## Why This Exists

During idea 355 Step 4 classification, `00181` still failed with
`RUNTIME_NONZERO` after the address-valued memory and external call-argument
publication repairs. The first bad fact is in generated AArch64 for the
same-module recursive `Hanoi` path: prepared call plans place pointer formals
such as `%p.source` and `%p.dest` in callee-saved homes `x20` and `x21`, but
generated code initializes those homes only on the base-case path before
`Move`. The recursive path then passes uninitialized `x20`/`x21` into recursive
`Hanoi` and `Move` calls.

This is adjacent to earlier call-argument preservation work, but distinct from
idea 355's address-valued external-call publication boundary.

## In Scope

- Localize the formal-home publication boundary for pointer formals in
  same-module recursive functions.
- Repair AArch64 lowering so callee-saved pointer-formal homes are populated on
  every path that later reloads or passes them.
- Add focused backend coverage for recursive or same-module call publication
  using pointer formals assigned to preserved homes.
- Prove `c_testsuite_aarch64_backend_src_00181_c` advances or passes, while
  preserving already repaired address-valued call-argument cases.

## Out Of Scope

- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- External-call symbol or address-valued memory publication already covered by
  ideas 354 and 355.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic stack
  work, or scalar compare/select residuals.
- Expectation, unsupported, runner, timeout, CTest-registration, or proof-log
  changes.

## Acceptance Criteria

- The first bad fact is localized to a same-module pointer-formal home
  publication path, not a source-name or testcase-specific branch.
- Focused backend coverage fails before the repair and passes after it for a
  pointer formal whose preserved home is consumed across recursive or
  same-module calls.
- `00181` advances or passes without expectation, runner, timeout, or
  filename-specific changes.
- Prior representatives from idea 355, especially `00170` and `00189`, remain
  passing in the focused proof subset.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, `Move`, one callee-saved register, one stack
  offset, or one emitted instruction neighborhood instead of repairing
  pointer-formal home publication semantics;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration;
- claims progress through helper renames or classification-only notes while
  recursive paths can still pass uninitialized callee-saved pointer homes;
- broadens into semantic string-load repair, frontend admission, ABI
  composite handling, or variadic/scalar residuals without a lifecycle split;
- regresses address-valued call-argument publication for the already passing
  `00170` and `00189` representatives.
