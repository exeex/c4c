# Frontend-to-BIR Legacy String Lookup Closure Gate

Status: Open (Blocked)
Created: 2026-05-12

Depends On:
- `ideas/closed/188_lir_bir_freeze_closure_gate.md`
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`
- `ideas/closed/193_hir_constructor_member_owner_structured_lookup_closure.md`
- `ideas/closed/194_bir_global_memory_provenance_linknameid_expansion.md`

## Goal

Close the second frontend-to-BIR legacy string lookup audit as the final
pre-backend-restart authority gate.

This gate should confirm that parser, sema, HIR, LIR, and BIR retained strings
are either structured-authority mirrors, display/output text, diagnostics,
route-local handles, ABI/final spelling, or explicit no-metadata
compatibility.

## Why This Idea Exists

After the LIR/BIR freeze batch, a broader frontend-to-BIR review found several
remaining legacy string lookup surfaces outside the first LIR/BIR scope:
structured call argument payload, function-signature byval text parsing, HIR
compile-time rendered registries, HIR constructor/member owner fallback, and
additional BIR global memory provenance routes.

Backend restart should wait until those surfaces are closed or consciously
fenced.

## Blocked State

This gate is inactive while its remaining Step 3 HIR blocker is being resolved:

- `ideas/open/201_hir_template_registry_structured_generated_paths.md`

The generated-member payload blocker was resolved by
`ideas/closed/202_hir_generated_member_payload_structured_miss.md`.

Do not advance this idea to milestone validation, closure, or backend restart
readiness until the remaining blocker is either completed or explicitly fenced
by a supervisor-approved lifecycle decision.

## In Scope

- Review completed ideas 188 and 190-194.
- Produce a frontend-to-BIR closure ledger covering parser token spelling,
  sema consteval/type utils, HIR compile-time state, HIR object/member owner
  lookup, HIR-to-LIR call/type metadata, LIR call payload, and BIR lowering.
- Confirm no high-risk metadata-rich path recovers semantic identity through
  rendered strings after a complete structured miss.
- Confirm retained raw/no-id compatibility has explicit owner and removal
  condition.
- Run broad validation appropriate for a pre-backend-restart milestone.

## Out Of Scope

- Starting backend restart.
- Removing all strings from printers, assemblers, diagnostics, or route-local
  SSA/slot/block handles.
- Rewriting parser/sema/HIR type systems.
- Accepting expectation downgrades or testcase-shaped shortcuts as closure.

## Acceptance Criteria

- A closure ledger classifies the remaining frontend-to-BIR legacy string
  lookup surfaces.
- Full-suite or milestone-appropriate validation is green, or baseline changes
  are explicitly handled by regression guard policy.
- Any remaining blocker is converted into a new narrow open idea before backend
  restart proceeds.
- The gate clearly states whether backend restart is allowed next.

## Reviewer Reject Signals

- The gate ignores known HIR rendered registry or constructor/member owner
  fallback paths.
- The gate treats route-local strings as semantic identity without evidence.
- Broad validation is skipped without justification.
- Backend restart implementation begins inside this closure gate.
