# RV64 Loop-2e Indirect Store Writeback Runtime Reactivation Runbook

Status: Active
Source Idea: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md

## Purpose

Reactivate idea 657 after the baseline review accepted the previous noisy
backend proof context. Confirm that the representative `loop-2e.c` RV64
runtime repair remains valid and prove whether the source idea is now
closure-ready.

## Goal

Show that `tests/c/external/gcc_torture/src/loop-2e.c` still matches clang
under RV64 object runtime after the pointer-value store repair, and that the
accepted backend regression baseline has no new failures for this idea.

## Core Rule

Do not reopen the completed `%t23` source-publication route or special-case
`loop-2e.c`, `%t23`, `q[39]`, or callee `f`. Treat the existing pointer-value
store repair as valid only if fresh semantic/prepared/RV64/runtime evidence
and backend proof support it.

## Read First

- `ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md`
- `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
  if present
- `test_before.log` and `test_after.log` only if the supervisor establishes
  them as the current matching baseline/proof pair

## Current Target

- Representative source:
  `tests/c/external/gcc_torture/src/loop-2e.c`
- Existing repair boundary: RV64 pointer-value indirect store emission through
  explicit prepared `base=pointer_value` access facts
- Acceptance surface: representative runtime comparison plus supervisor-chosen
  backend regression proof

## Non-Goals

- Do not implement new indirect-store, postincrement, branch-authority,
  stack-destination fan-in, byval, object-emission, AArch64, CLI, or LLVM
  torture work under this plan.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- Do not claim closure from stale Step 3 logs without refreshing the
  representative runtime and accepted regression proof.

## Working Model

The parked runbook recorded that the representative `loop-2e.c` RV64
object-runtime comparison passed after the pointer-value store repair, but the
backend proof was too noisy for acceptance. The current supervisor context says
the baseline review was accepted with a full-suite candidate improvement and
no new failures. This runbook therefore starts from proof refresh and closure
readiness, not from another implementation search.

## Execution Rules

- Keep evidence under `build/agent_state/657_*`.
- Preserve the existing semantic/prepared contract for explicit
  pointer-value stores; do not infer destination authority from testcase names,
  final assembly, value IDs, or runtime behavior alone.
- If fresh evidence shows the representative runtime no longer passes, stop at
  the first changed owner and record whether this plan needs repair or
  deactivation.
- For any code-changing packet, return to the full validation ladder:
  `cmake --build --preset default`, focused representative proof, then the
  supervisor-selected backend regression subset in `test_after.log`.

## Ordered Steps

### Step 1: Refresh Representative Runtime Proof

Goal: Reconfirm the current `loop-2e.c` RV64 object-runtime result and the
prepared pointer-value store facts that justify it.

Primary target:

- `tests/c/external/gcc_torture/src/loop-2e.c`

Actions:

- Refresh BIR, prepared-BIR, ASM, object, disassembly, and runtime comparison
  evidence for the representative source.
- Confirm that the explicit prepared pointer-value access fact still drives
  the caller-visible indirect store, while local cursor writeback remains
  separate.
- Confirm that the completed `%t23` pointer-source publication and branch RHS
  authority have not regressed.
- Record a short summary under
  `build/agent_state/657_step1_reactivation_runtime_proof/`.

Completion check:

- Fresh evidence shows the representative RV64 object runtime matches clang,
  or names a precise changed owner without expectation or policy changes.

### Step 2: Prove Backend Regression Safety

Goal: Compare the refreshed route against the accepted backend baseline with
no new failures.

Actions:

- Run the supervisor-selected build and backend regression proof.
- Keep canonical proof in `test_after.log` unless the supervisor delegates a
  different artifact.
- Compare against the matching accepted baseline/proof context from the
  supervisor.
- Record any remaining red rows as known unrelated baseline state or as a
  precise follow-up owner only when evidence proves they are outside idea 657.

Completion check:

- Backend proof shows no new backend regressions for idea 657 under the
  accepted baseline comparison.

### Step 3: Decide Closure Readiness

Goal: Leave enough lifecycle evidence for the supervisor or plan owner to
close, deactivate, or split the idea.

Actions:

- Summarize whether the source idea acceptance criteria are satisfied:
  focused boundary evidence, representative runtime result, and regression
  proof.
- If all criteria are met, mark the current packet complete in `todo.md` and
  request lifecycle close review.
- If not, record the exact blocker and whether it is an implementation owner,
  proof-gap owner, or separate follow-up idea.

Completion check:

- The next lifecycle action is unambiguous: close idea 657, continue with a
  named repair packet, or deactivate with a precise blocker.

## Completion Criteria

- Focused evidence still names the RV64 pointer-value indirect-store boundary.
- The representative `loop-2e.c` RV64 object-runtime comparison matches clang,
  or fails closed at a precise downstream owner.
- Backend regression proof shows no new failures in the supervisor's accepted
  baseline comparison.
