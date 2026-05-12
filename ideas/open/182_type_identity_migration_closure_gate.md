# Type Identity Migration Closure Gate

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/open/178_global_aggregate_layout_structured_boundary.md`
- `ideas/open/179_byval_copy_layout_structured_boundary.md`
- `ideas/open/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md`
- `ideas/open/181_function_pointer_signature_type_identity.md`

Parent Ideas:
- `ideas/closed/172_type_identity_authority_audit.md`
- `ideas/closed/173_aggregate_layout_identity_structured_boundary.md`
- `ideas/closed/174_aggregate_abi_classification_structured_facts.md`
- `ideas/closed/175_hir_typespec_ref_structured_equivalence.md`
- `ideas/closed/176_lir_type_ref_structured_equality.md`
- `ideas/closed/177_template_record_owner_structured_identity.md`

## Goal

Close the type-identity migration wave as a coherent milestone after the
post-audit implementation slices finish.

This gate should confirm which type domains are structured, which rendered
type spellings remain output/diagnostic/ABI text, and which compatibility
bridges are intentionally retained for no-metadata inputs.

## Why This Idea Exists

Type identity spans parser syntax, sema canonicalization, HIR lowering, LIR
type refs, BIR ABI/layout facts, and backend preparation. Without a closure
gate, the project can keep chasing isolated type-string surfaces without
deciding whether the migration has reached a stable boundary.

## In Scope

- Review closed ideas 172-181 and produce a type-domain closure ledger.
- Run broad validation, including full suite unless an explicit baseline
  policy says otherwise.
- Confirm retained rendered type strings are output, diagnostics, ABI spelling,
  or explicit no-metadata compatibility bridges.
- Confirm structured facts own the selected aggregate layout, aggregate ABI,
  HIR type-ref, LIR type-ref, template record owner, global aggregate, byval
  copy, AArch64/direct-LIR, and function-pointer signature paths.
- Decide whether any remaining type-identity blocker needs a new narrow idea,
  or whether the next migration theme should begin.

## Out Of Scope

- Starting the next migration theme.
- Rewriting the full type system or removing all `TypeSpec` syntax payloads.
- Changing output/diagnostic type spelling just to reduce strings.
- Accepting weak tests or unsupported downgrades as closure.

## Acceptance Criteria

- The post-type-identity wave has an accepted validation/baseline result.
- A type identity closure ledger documents syntax payload, resolved type
  identity, layout identity, ABI facts, display spelling, and compatibility.
- No high-risk spelling-based type authority remains unclassified.
- Any remaining blockers are converted into explicit follow-up ideas.

## Reviewer Reject Signals

- The gate accepts failures without explaining baseline status.
- `TypeSpec` syntax payload and resolved type identity are conflated.
- Rendered type spelling remains semantic authority without classification.
- The gate starts unrelated implementation work.
