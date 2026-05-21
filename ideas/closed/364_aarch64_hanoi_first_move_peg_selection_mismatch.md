# AArch64 Hanoi First Move Peg Selection Mismatch

Status: Closed
Created: 2026-05-21
Split From: ideas/closed/363_aarch64_prepared_select_condition_join_stale_reload.md
Closed: 2026-05-21

## Goal

Repair the next AArch64 `00181` residual after prepared scan joins terminate
correctly: the first printed Hanoi move places disk `1` on `B` instead of the
expected `C`.

## Why This Exists

Idea 363 repaired stale prepared select/condition join reloads in generated
`Move`. Live generated `00181` no longer reloads stale branch operands at the
source and destination scan joins, and the previous segmentation fault advanced
to a runtime output mismatch.

The new first bad fact is in the first subsequent Hanoi state. Actual output
has `B: 0 0 0 1` and `C: 0 0 0 0`; expected output has `B: 0 0 0 0` and
`C: 0 0 0 1`. This points past the stale scan-join reload owner and needs a
fresh localization around peg selection, recursive call state, or the later
store-address/value publication path.

## In Scope

- Localize the first wrong state transition after the source and destination
  scans now terminate correctly.
- Compare semantic BIR, prepared BIR, and generated AArch64 around the first
  recursive move and the first store/update that places disk `1`.
- Determine whether the wrong peg is caused by argument/peg selection,
  recursive call state preservation, store-address selection, store-value
  publication, or another concrete same-owner boundary.
- Repair the general backend rule for the localized owner without matching
  `00181`, `Move`, one peg name, one tower global, one block label, one stack
  offset, or one emitted register.
- Add focused backend coverage for the localized owner shape when a small
  backend contract can represent it.
- Prove `00181` advances beyond the first-move `B`-instead-of-`C` mismatch or
  passes, while keeping the repairs from ideas 360 through 363 stable.

## Out Of Scope

- Reopening prepared select/condition stale scan-join reloads from idea 363
  unless the exact stale reload failure reappears.
- Reopening pointer-derived load/address scaling from idea 362 except to keep
  it stable.
- Reopening materialized pointer-addressed store writeback from idea 361 except
  to keep it stable.
- Reopening the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Reopening recursive formal post-call repairs from ideas 357, 358, and 359
  without fresh first-bad-fact evidence that the new mismatch belongs there.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, expectation changes, unsupported classifications, runner
  behavior, timeout policy, CTest registration, or proof-log policy.

## Acceptance Criteria

- The first wrong Hanoi move is localized to a concrete semantic-BIR,
  prepared-BIR, or generated-AArch64 boundary with expected and actual peg
  identity/state evidence.
- The repair is a general backend capability fix for the localized owner, not a
  named-case patch for `00181` or Hanoi output text.
- Focused backend coverage exists for the repaired owner shape when practical;
  if direct backend coverage is not practical, `todo.md` records why and names
  the stronger generated-code or runtime evidence used instead.
- `00181` advances beyond the first-move `B`-instead-of-`C` runtime mismatch
  or passes without expectation, runner, timeout, or filename-specific changes.
- The stale join reload repair remains stable: generated `00181` must not
  reintroduce stale `.LBB193_18` or `.LBB193_25` branch-operand reloads over
  the published false/zero value.
- The idea 362 pointer-derived address scaling repair remains stable, including
  distinct index and scale registers in the representative multiply shape.
- `00170`, `00189`, and the focused backend contracts for nearby memory,
  prepared-BIR, and AArch64 dispatch shapes remain passing.

## Completion Note

The first-move mismatch was localized to same-register select-materialized
predecessor edge publication. Commit `c0466384e` repaired the general
same-register RHS edge publication path so the generated compare in `00181`
loads the actual edge value (`ldr w13, [sp, #112]`, `cmp w13, #0`,
`cset w13, eq`) instead of preserving the stale no-op `mov x13, x13`.

Focused before/after regression guard for the idea scope passed: the seven-test
subset improved from 6/7 to 7/7, resolving
`c_testsuite_aarch64_backend_src_00181_c` with no new failures. Supervisor
broader backend guard also passed (`ctest --test-dir build -j
--output-on-failure -R '^backend_'`, 141/141). The hook full-suite candidate
was not accepted as a baseline because it introduced unrelated new failures
outside this idea's scope.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Move`, Hanoi tower globals, peg letters, output text,
  one block label, one stack offset, one ABI register, or one emitted
  instruction neighborhood instead of repairing the localized backend owner;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, output
  expectation rewrites, or pass-count movement while the first move still
  places disk `1` on `B` instead of `C`;
- reopens the prepared select/condition stale reload, pointer-derived
  load-address scaling, materialized pointer store writeback, direct
  `LoadGlobal` select-store, recursive formal post-call, semantic string-load,
  frontend admission, ABI composite, or variadic/floating owners without fresh
  evidence that the first wrong move belongs there;
- hides the same wrong-peg behavior behind a new abstraction name or a broader
  rewrite that cannot identify the first incorrect boundary.
