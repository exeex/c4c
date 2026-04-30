# LIR BIR Backend Aggregate Layout Type Decl Text Bridge Cleanup

Status: Closed
Created: 2026-04-29
Closed: 2026-04-30

Parent Ideas:
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Retire semantic layout authority that still crosses the LIR to BIR/backend
boundary through final `%type` spelling and legacy textual type-declaration
bodies when structured aggregate layout carriers are available.

## Why This Idea Exists

Idea 131 Step 4 found that later-IR link-visible symbols, block labels,
value names, local slots, diagnostics, dumps, and final emitted text are
already structured, route-local, or legitimate final/display spelling. The
remaining suspicious later-IR family is aggregate/type layout authority:
structured `LirStructDecl` and `StructNameId` layouts are preferred and parity
checked when present, but layout lookup is still driven by final type spelling
and can fall back to legacy text when structured layout is missing or parity
cannot be used.

Suspicious paths:

- `LirModule::type_decls`
- `TypeDeclMap`
- `compute_aggregate_type_layout()`
- `lookup_backend_aggregate_type_layout()`
- `BackendStructuredLayoutTable`
- `StructuredTypeSpellingContext`
- aggregate and global initializer layout parsing that treats final `%type`
  text or legacy type-decl bodies as layout authority

## Scope

- Trace LIR type declarations, structured struct declarations, BIR aggregate
  layout import, backend structured layout tables, and global initializer
  layout consumers across the LIR/BIR/backend boundary.
- Make `LirStructDecl`, `StructNameId`, structured type refs, or an equivalent
  structured aggregate layout table the semantic authority when producer data
  is available.
- Keep final `%type` spelling only as emitted artifact spelling, diagnostics,
  dumps, or explicit compatibility fallback with structured-primary behavior
  and mismatch visibility.
- Preserve parity checks while removing any path where legacy textual
  type-decl parsing silently overrides or substitutes for available structured
  layout data.
- Add focused coverage for structured-present, structured-missing legacy
  fallback, mismatch, aggregate initializer, and global initializer paths.

## Out Of Scope

- Removing final LLVM or assembly type spellings that are emitted artifacts.
- Bulk deleting legacy `type_decls` before raw or hand-built LIR compatibility
  has an explicit fallback contract.
- Changing Parser, Sema, or HIR owner/member/template lookup cleanup; those
  paths are covered by ideas 132 through 137.
- Reworking unrelated link-name, block-label, value-name, local-slot,
  diagnostic, dump, inline asm, register-name, or final assembly text paths.

## Acceptance Criteria

- Aggregate layout decisions prefer structured LIR/BIR/backend layout carriers
  over final `%type` spelling whenever structured producer data exists.
- Legacy textual type-declaration parsing is quarantined as compatibility-only,
  with clear mismatch/fallback reporting instead of silent semantic authority.
- Backend aggregate and global initializer layout consumers no longer need to
  rederive authoritative layout from final type spelling when structured layout
  data is present.
- Focused tests cover same-feature aggregate and initializer cases rather than
  proving only one named testcase.

## Closure Notes

Completed through active runbook Step 6. Backend aggregate lookup now reports
structured layout selection, legacy fallback use, and structured/text mismatch
state through the central lookup contract, with aggregate and global
initializer consumers using that status-aware route while preserving explicit
legacy fallback for raw or hand-built LIR.

Close proof used the matching `^backend_` regression scope recorded in
`test_before.log` and `test_after.log`: 109/109 runnable backend tests passed
before and after, with 12 disabled MIR trace tests unchanged and no new
failures under the regression guard.
