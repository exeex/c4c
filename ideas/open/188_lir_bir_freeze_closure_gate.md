# LIR/BIR Freeze Closure Gate

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `ideas/closed/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/closed/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/closed/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/closed/187_bir_memory_provenance_global_handle_cleanup.md`
- `ideas/closed/189_direct_call_no_prototype_variadic_signature_mismatch.md`
- `ideas/open/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md`

## Goal

Close the LIR/BIR freeze wave as the final pre-backend-restart gate.

This gate should confirm that LIR/BIR/backend-prealloc identity authority is
classified, structured facts own generated metadata-rich paths, compatibility
fallbacks are fenced, and backend restart can proceed without reopening the
same string-authority migration.

## Why This Idea Exists

The project is intentionally delaying backend restart until parser, sema, HIR,
LIR, and BIR identity surfaces converge. Ideas 183-187 formed the first LIR/BIR
convergence batch, 189 fixed a direct-call blocker found during validation, and
190/191/194 cover the remaining LIR/BIR string-lookup surfaces found by the
second frontend-to-BIR audit. A closure gate prevents the next backend plan
from starting while freeze blockers are still implicit.

## In Scope

- Review completed ideas 183-187, 189-191, and 194, then produce a LIR/BIR
  freeze ledger.
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

- 2026-05-12: Milestone validation was blocked by direct-call signature
  mismatches in old-style/no-prototype and variadic C calls. The blocker was
  split into
  `ideas/closed/189_direct_call_no_prototype_variadic_signature_mismatch.md`
  and closed after the semantic repair landed in `3ea8793b6 preserve
  no-prototype direct-call signatures`. The freeze gate is reactivated at
  milestone validation with full-suite proof available at `3137/3137`.

## Reviewer Reject Signals

- The gate claims freeze while direct-call or global-symbol generated paths
  still rely on rendered text.
- The gate treats printer/assembler/output strings as semantic identity without
  classification.
- Validation is skipped or only narrow tests are used for a milestone closure
  without justification.
- Backend restart work begins inside the closure gate.
