# LIR/BIR Freeze Closure Gate

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/open/183_lir_bir_backend_freeze_authority_audit.md`
- `ideas/open/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md`

## Goal

Close the LIR/BIR freeze wave as the final pre-backend-restart gate.

This gate should confirm that LIR/BIR/backend-prealloc identity authority is
classified, structured facts own generated metadata-rich paths, compatibility
fallbacks are fenced, and backend restart can proceed without reopening the
same string-authority migration.

## Why This Idea Exists

The project is intentionally delaying backend restart until parser, sema, HIR,
LIR, and BIR identity surfaces converge. Ideas 183-187 are the proposed final
LIR/BIR convergence batch. A closure gate prevents the next backend plan from
starting while freeze blockers are still implicit.

## In Scope

- Review completed ideas 183-187 and produce a LIR/BIR freeze ledger.
- Confirm direct-call signatures, global/type declaration tables, direct
  symbol identity, memory provenance global handles, and prealloc route-local
  names are classified.
- Confirm retained strings are display/output, diagnostics, route-local
  handles, ABI/final spelling, or explicit no-metadata compatibility.
- Run broad validation appropriate for a milestone, normally full suite unless
  baseline policy requires otherwise.
- Decide whether backend restart can proceed or whether a narrow blocker idea
  must be created first.

## Out Of Scope

- Starting the backend restart itself.
- Rewriting target MIR, assemblers, linkers, or object emission.
- Removing all final spelling strings from backend output layers.
- Accepting testcase expectation downgrades as freeze progress.

## Acceptance Criteria

- A freeze closure ledger documents each LIR/BIR identity domain and retained
  compatibility boundary.
- Broad validation is green or any baseline difference is explicitly accepted
  through the regression guard workflow.
- No high-risk generated-path string authority remains unclassified.
- Any remaining blocker is captured as a new open idea before backend restart.

## Lifecycle Notes

- 2026-05-12: Milestone validation is blocked, not closed. The accepted
  `test_before.log` baseline was green at 3137 passed / 0 failed, while the
  Step 3 full-suite `test_after.log` result has 3128 passed / 9 failed. The
  common observed failure is
  `LirCallOp.callee_signature: structured callee signature does not match call
  arguments`, including old-style/no-prototype and variadic direct-call cases.
  This gate is parked until `ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md`
  resolves the direct-call signature mismatch blocker.

## Reviewer Reject Signals

- The gate claims freeze while direct-call or global-symbol generated paths
  still rely on rendered text.
- The gate treats printer/assembler/output strings as semantic identity without
  classification.
- Validation is skipped or only narrow tests are used for a milestone closure
  without justification.
- Backend restart work begins inside the closure gate.
