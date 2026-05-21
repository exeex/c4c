# AArch64 Recursive Stack-Preserved Pointer Formal Post-Call Overwrite

Status: Open
Created: 2026-05-21
Split From: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md

## Goal

Repair recursive or same-module call paths where a pointer formal's
stack-preserved home is overwritten from a clobbered ABI argument register
after an intervening call, then reloaded for a later recursive or helper call.

## Why This Exists

Idea 358 repaired the scalar `%p.n` post-call preservation boundary exposed by
`00181`: generated AArch64 no longer computes the later decrement from
clobbered `w0`.

After that scalar repair, `00181` still fails with `RUNTIME_NONZERO`, but the
first bad fact moved again. The remaining path overwrites the stack-preserved
home for pointer formal `%p.spare` from clobbered `x3` after a call and later
reloads that bad stack slot for another recursive call. This is not the
callee-saved pointer-formal home publication repaired by idea 357, because the
bad home is stack-preserved and is corrupted after a clobbering call.

## In Scope

- Localize the prepared stack home for pointer formals such as `%p.spare` that
  remain live after recursive or helper calls.
- Repair AArch64 lowering so post-call preservation or reload logic does not
  republish a stack-preserved pointer formal from a clobbered ABI argument
  register.
- Add focused backend coverage for a stack-preserved pointer formal consumed
  after one or more calls on a recursive or same-module path.
- Prove `c_testsuite_aarch64_backend_src_00181_c` advances or passes while
  preserving the scalar-formal post-call repair from idea 358 and the
  callee-saved pointer-formal repair from idea 357.

## Out Of Scope

- Scalar formal post-call reloads for values such as `%p.n`, already handled by
  idea 358.
- Callee-saved pointer-formal home publication for `%p.source` and `%p.dest`,
  already closed under idea 357.
- Address-valued memory and call-argument publication already closed under
  idea 355.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated scalar compare/select residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a stack-preserved pointer formal being
  overwritten or reloaded across a post-call boundary, not a source-name or
  testcase-specific branch.
- Focused backend coverage fails before the repair and passes after it for a
  stack-preserved pointer formal whose original ABI register is clobbered by an
  intervening call.
- `00181` advances or passes without expectation, runner, timeout, or
  filename-specific changes.
- The idea 358 scalar-formal repair remains stable: post-call scalar consumers
  still reload preserved homes instead of using clobbered ABI argument
  registers.
- The idea 357 callee-saved pointer-formal repair remains stable:
  recursive/helper-call paths still publish pointer formals into their
  callee-saved homes before consumers use those homes.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, `Move`, `%p.spare`, one stack offset, one ABI
  register, or one emitted instruction neighborhood instead of repairing
  stack-preserved pointer formal post-call semantics;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while generated code can still overwrite a
  live stack-preserved pointer formal home from a clobbered ABI argument
  register after a call;
- reopens scalar formal reloads, callee-saved pointer-formal publication,
  address-valued publication, semantic string loads, frontend admission, ABI
  composite work, or variadic/floating residuals without fresh first-bad-fact
  evidence and a lifecycle split;
- regresses the already repaired scalar `%p.n` post-call reload behavior,
  pointer-formal callee-saved home publication, `00170`, or `00189`.
