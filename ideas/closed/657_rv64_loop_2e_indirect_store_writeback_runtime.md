# RV64 Loop-2e Indirect Store Writeback Runtime

Status: Closed
Type: Implementation
Parent: `ideas/closed/653_stack_carried_pointer_source_publication_materialization.md`
Related:
- `ideas/closed/653_stack_carried_pointer_source_publication_materialization.md`
- `build/agent_state/653_step4a_loop_t23_runtime_probe/summary.md`
Owning Layer: RV64 callee indirect-store and postincrement writeback lowering
after pointer branch source materialization
Queue Order: 57
Proof Surface: `tests/c/external/gcc_torture/src/loop-2e.c` after object
emission succeeds.

## Goal

Repair the downstream RV64 runtime owner in `src/loop-2e.c` where callee `f`
updates local `q` storage instead of storing the computed pointer through the
caller-provided `*q++` destination.

## Why This Exists

Idea 653 closed the `%t23` pointer-source boundary: semantic and prepared BIR
publish `%t23 = bir.add ptr %t21, 156`, branch RHS authority is available for
the refreshed `%t23` home, and RV64 object emission exits 0. Runtime still
aborts under qemu while clang exits 0. Step 4A evidence classifies the first
remaining owner as callee indirect-store / `*q++` writeback lowering, because
callee `f` updates the local stack home for `q` at `0(sp)` instead of storing
through the caller array, and `main` later reloads unchanged `q[39]`.

## In Scope

- Refresh BIR, prepared-BIR, ASM, object, disassembly, and one-case runtime
  evidence for `src/loop-2e.c`.
- Trace callee `f`'s indirect-store and postincrement writeback path from the
  source pointer through prepared value homes and RV64 stores.
- Repair the general indirect-store or postincrement writeback rule only when
  explicit semantic/prepared facts prove the destination and updated pointer.
- Preserve the completed `%t23` pointer-source publication, branch authority,
  and RV64 materialization chain from idea 653.

## Out Of Scope

- Reopening `%t23` semantic producer publication, clobber safety, or branch RHS
  authority unless fresh evidence proves a regression.
- Reopening RV64 fused pointer branch terminator admission.
- Changing runtime policy, unsupported markers, expectation files, allowlists,
  timeouts, or pass/fail accounting.
- Special-casing `src/loop-2e.c`, `%t23`, `q[39]`, or callee `f` by name.

## Acceptance Criteria

- Focused evidence names the exact prepared/RV64 indirect-store or
  postincrement writeback boundary that loses the caller-visible update.
- The repaired route either makes the one-case RV64 runtime match clang or
  fails closed with a precise unsupported diagnostic at the proven downstream
  owner.
- Backend regression proof shows no new backend failures.

## Deactivation Notes

- 2026-07-10: Active runbook deactivated before closure. Step 3 evidence
  under `build/agent_state/657_step3_representative_pointer_value_store/`
  showed the representative `loop-2e.c` RV64 object-runtime comparison passing
  against clang after the pointer-value store repair, but Step 4 backend
  baseline/proof remained too noisy to use as acceptance evidence.
- `test_baseline.log` showed broad existing failures across RV64 backend
  dump/codegen/runtime, prepared BIR/CLI, and llvm torture coverage. Do not
  treat this deactivation as idea completion; reactivate only after the
  baseline is understood well enough to prove no new backend regressions.

## Closure Notes

- 2026-07-10: Closed after reactivation runbook
  `657_rv64_loop_2e_indirect_store_writeback_runtime`. Fresh representative
  evidence under
  `build/agent_state/657_step1_reactivation_runtime_proof/summary.md` shows
  `loop-2e.c` RV64 object runtime matching clang, with the explicit prepared
  `base=pointer_value` access fact driving the caller-visible store while
  local cursor writeback remains separate.
- The completed `%t23` source publication and branch RHS authority stayed
  intact in the refreshed evidence.
- Close-time full-suite regression comparison reused canonical
  `test_before.log` and `test_after.log`. Both logs report 3386 passed and 11
  failed out of 3397 with the same failed tests; the regression guard passed in
  non-decreasing mode with zero new failures against the accepted red baseline.

## Reviewer Reject Signals

- Reject any change that weakens runtime comparison, expectation files,
  unsupported markers, allowlists, or timeout accounting.
- Reject reclassifying the failure as missing `%t23` source publication while
  current BIR/prepared/RV64 evidence still shows `%t23 = bir.add ptr %t21, 156`
  and available branch RHS authority.
- Reject named-case shortcuts for `src/loop-2e.c`, `%t23`, `q[39]`, or callee
  `f` instead of a general indirect-store or postincrement writeback rule.
- Reject storing to the local pointer variable home when the semantic
  destination is the caller-provided pointee.
- Reject helper renames or diagnostic-only edits that leave the same qemu abort
  behind a new label.
