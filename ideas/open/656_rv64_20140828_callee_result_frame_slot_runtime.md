# RV64 20140828 Callee Result Frame-Slot Runtime

Status: Open
Type: Implementation
Parent: `ideas/closed/653_stack_carried_pointer_source_publication_materialization.md`
Related:
- `ideas/closed/653_stack_carried_pointer_source_publication_materialization.md`
- `build/agent_state/653_step4_representative_integration/summary.md`
Owning Layer: RV64 callee result or frame-slot value correctness after pointer
branch source materialization
Queue Order: 56
Proof Surface: `tests/c/external/gcc_torture/src/20140828-1.c` after object
emission succeeds.

## Goal

Repair the downstream RV64 runtime owner that remains after the `%t6` branch
pointer source in `src/20140828-1.c` is explicitly published, materialized, and
accepted by object emission.

## Why This Exists

Idea 653 closed the pointer-source authority boundary for `%t6`: semantic BIR
publishes `%t6 = bir.add ptr %lv.a.0, 2`, prepared branch RHS authority is
available, RV64 materializes the selected source at the branch, and object
emission exits 0. The one-case runtime still aborts under qemu while clang
exits 0. Step 4 evidence points away from missing `%t6` publication and toward
callee/result or frame-slot value correctness around the preceding
`f(a, 1, &d)` path.

## In Scope

- Refresh BIR, prepared-BIR, ASM, object, disassembly, and runtime evidence for
  `src/20140828-1.c` after idea 653.
- Identify the first wrong value or store/load boundary before the abort.
- Repair callee result propagation, frame-slot value preservation, or related
  RV64 value-home materialization only when the evidence proves that boundary.
- Preserve the completed `%t6` pointer-source publication and materialization
  chain from idea 653.

## Out Of Scope

- Reopening stack-carried pointer source publication or branch RHS authority
  unless fresh evidence proves a regression in those facts.
- Reopening RV64 fused pointer branch terminator admission.
- Changing runtime policy, unsupported markers, expectation files, allowlists,
  timeouts, or pass/fail accounting.
- Special-casing `src/20140828-1.c`, `%t6`, or the final branch compare.

## Acceptance Criteria

- Focused evidence names the exact first downstream wrong-value boundary for
  `src/20140828-1.c`.
- The repaired route either makes the one-case RV64 runtime match clang or
  fails closed with a precise unsupported diagnostic at the proven downstream
  owner.
- Backend regression proof shows no new backend failures.

## Reviewer Reject Signals

- Reject any change that claims progress by weakening the runtime comparison,
  expectation files, unsupported markers, allowlists, or timeout accounting.
- Reject re-inference of `%t6` source identity from stack offsets, final
  assembly shape, source spelling, or testcase name.
- Reject branch-lowering rewrites that obscure the already-explicit `%t6`
  source-selection authority without fixing the downstream wrong value.
- Reject named-case shortcuts for `src/20140828-1.c`, `f(a, 1, &d)`, or `%t6`
  that do not establish a general callee/result or frame-slot value rule.
- Reject diagnostic-only or helper-rename changes that leave the same qemu abort
  behind a new label.
