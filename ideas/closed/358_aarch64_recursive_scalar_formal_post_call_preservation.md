# AArch64 Recursive Scalar Formal Post-Call Preservation

Status: Closed
Created: 2026-05-21
Split From: ideas/closed/357_aarch64_recursive_pointer_formal_home_publication.md

## Closure Notes

Closed on 2026-05-22 after reactivation Step 1 refreshed the current first bad
fact and found no live scalar-formal post-call preservation owner. The focused
proof passed `backend_cli_aarch64_asm_external_return_add_smoke`, `00170`,
`00181`, and `00189`; `00181` is green on the current tree.

Close gate used the existing matching canonical `test_before.log` and
`test_after.log` focused logs. The strict monotonic checker rejected equal pass
counts, as expected for a lifecycle-only close, and the documented
`--allow-non-decreasing-passed` maintenance mode passed with 4/4 before and 4/4
after.

## Historical Parked Notes

The scalar formal post-call preservation scope is complete.

Commit `0a7ef5764` repaired the `%p.n` boundary for recursive paths that used a
clobbered ABI argument register after calls. Generated `00181` now stores the
incoming scalar formal to its stack home before the first recursive call and
reloads that preserved home before the later decrement instead of computing
from clobbered `w0`.

Earlier parked proof passed backend contracts,
`backend_cli_aarch64_asm_external_return_add_smoke`, `00170`, and `00189`.
At that time, `00181` advanced past the scalar formal first bad fact but still
failed with `RUNTIME_NONZERO` because stack-preserved pointer formal
`%p.spare` was later overwritten from clobbered `x3` and reloaded for another
recursive call. That residual was outside this scalar-formal idea and was split
to
`ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md`.

## Goal

Repair recursive or same-module call paths where a scalar formal must be
reloaded from its prepared preserved home after calls clobber the incoming ABI
argument register.

## Why This Exists

Idea 357 repaired the original `00181` first bad fact: pointer formals
`%p.source` and `%p.dest` are now published into callee-saved homes `x20` and
`x21` before recursive/helper-call consumers use those homes.

After that repair, `00181` still fails with `RUNTIME_NONZERO`, but the first
bad fact moved. Generated AArch64 now reaches a later recursive path where
`n - 1` is computed from clobbered `w0` after calls instead of reloading the
preserved `%p.n` stack home. This is scalar formal post-call preservation, not
pointer-formal callee-saved home publication.

## In Scope

- Localize the prepared home for scalar formals such as `%p.n` that remain
  live after recursive or helper calls.
- Repair AArch64 lowering so post-call scalar uses reload the preserved formal
  home instead of using a clobbered ABI argument register.
- Add focused backend coverage for a scalar formal consumed after one or more
  calls on a recursive or same-module path.
- Prove `c_testsuite_aarch64_backend_src_00181_c` advances or passes while
  preserving the pointer-formal home publication repair from idea 357.

## Out Of Scope

- Pointer-formal callee-saved home publication already closed under idea 357.
- Address-valued memory and call-argument publication already closed under
  idea 355.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated scalar compare/select residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a scalar formal preserved-home/post-call
  reuse boundary, not a source-name or testcase-specific branch.
- Focused backend coverage fails before the repair and passes after it for a
  scalar formal whose original ABI register is clobbered by an intervening
  call.
- `00181` advances or passes without expectation, runner, timeout, or
  filename-specific changes.
- The idea 357 pointer-formal repair remains stable: recursive/helper-call
  paths still publish `%p.source` and `%p.dest` into their callee-saved homes
  before consumers use those homes.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, one scalar formal name, one source statement,
  one stack offset, one ABI register, or one emitted instruction neighborhood
  instead of repairing general post-call scalar formal preservation;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while generated code can still use a
  clobbered ABI argument register for a live scalar formal after a call;
- reopens pointer-formal callee-saved home publication, address-valued
  publication, semantic string loads, frontend admission, ABI composite work,
  or variadic/floating residuals without fresh first-bad-fact evidence and a
  lifecycle split;
- regresses the already repaired `00170`, `00189`, or idea 357 focused
  pointer-formal home publication coverage.
