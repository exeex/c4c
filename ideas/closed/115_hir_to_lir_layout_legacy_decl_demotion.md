# HIR To LIR Layout LegacyDecl Demotion

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

Parent Ideas:
- [112_lir_backend_legacy_type_surface_readiness_audit.md](/workspaces/c4c/ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md)
- [113_backend_struct_decl_layout_table_dual_path.md](/workspaces/c4c/ideas/open/113_backend_struct_decl_layout_table_dual_path.md)

## Goal

Demote selected HIR-to-LIR layout consumers from
`StructuredLayoutLookup::legacy_decl` authority to structured
`StructNameId` / `LirStructDecl` authority, while retaining legacy fallback for
incomplete coverage.

## Why This Idea Exists

Idea 109 added structured layout lookup observations, but several consumers
still use `legacy_decl` after parity observation. That keeps `TypeSpec::tag` and
`Module::struct_defs` as active layout authority inside HIR-to-LIR.

Once idea 112 identifies safe candidates and active BIR layout consumers have a
structured dual path, selected HIR-to-LIR consumers can switch to structured
layout facts first. Planned MIR rewrite work must not block this route; MIR
compile failures should be handled by compile-target exclusion, not by
migrating MIR legacy layout code.

## Scope

Use idea 112 as the source of truth. Candidate consumers include:

- const initializer aggregate/field lookup
- aggregate GEP/lvalue field-chain resolution
- variadic aggregate argument handling
- `va_arg` aggregate handling
- byval/byref aggregate call handling
- nested struct field lookup
- size/alignment helpers used by HIR-to-LIR lowering

## Execution Rules

- Prefer structured layout only where `StructNameId` is available and parity is
  stable.
- Keep `legacy_decl` fallback for incomplete structured coverage.
- Keep `TypeSpec::tag` bridge input intact until all consumers stop requiring
  it.
- Do not invest in `src/backend/mir/` cleanup as part of this route.
- If `src/backend/mir/` is the only blocker, remove the relevant `.cpp` files
  from the current compile target instead of changing MIR semantics.
- Record structured-vs-legacy mismatches.
- Preserve emitted LLVM and backend output.

## Acceptance Criteria

- Selected HIR-to-LIR layout consumers use structured layout facts first.
- `legacy_decl` is no longer semantic authority for cleaned paths.
- Legacy fallback remains available for unconverted paths.
- Aggregate initializer, GEP, variadic, `va_arg`, and byval/byref proof passes
  without output drift.

## Closure Notes

Closed after the idea 115 runbook completed Step 6 acceptance validation.

Completed scope:
- Const-initializer layout declaration authority was demoted away from
  `StructuredLayoutLookup::legacy_decl`.
- Structured-first alignment and size helpers were added for HIR-to-LIR layout
  consumers while preserving legacy fallback on missing coverage, conservative
  derivation, parity absence, or parity mismatch.
- Aggregate call, variadic aggregate argument, `va_arg`, and object-alignment
  consumers use structured-first helper authority before legacy fallback.
- Final HIR-to-LIR scan classified remaining `legacy_decl` reads as
  observation/carrier state, helper parity gates, or structured-first fallback
  paths rather than active semantic authority.

Review and validation:
- `review/idea115_layout_demotion_route_review.md` found the route aligned with
  the source idea, with no overfit or expectation downgrade.
- Step 6 full-suite logs `test_before.log` and `test_after.log` both recorded
  `100% tests passed, 0 tests failed out of 2980`.
- Close-time regression guard passed with
  `--allow-non-decreasing-passed`: before 2980/0/2980, after 2980/0/2980.

Deferred or out of scope:
- Structured union/member layout derivation can reduce conservative fallback
  later, but was not required for this demotion route.
- Broader raw LLVM type-text and nested-`LirStructDecl` coverage can reduce
  conservative size fallback later.
- Backend MIR cleanup remains out of scope for this idea.
