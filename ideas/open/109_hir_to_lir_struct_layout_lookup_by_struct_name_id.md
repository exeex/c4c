# HIR To LIR Struct Layout Lookup By StructNameId

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [106_shared_struct_name_table_for_lir_type_identity.md](/workspaces/c4c/ideas/closed/106_shared_struct_name_table_for_lir_type_identity.md)
- [107_lir_struct_name_id_type_ref_mirror.md](/workspaces/c4c/ideas/open/107_lir_struct_name_id_type_ref_mirror.md)

## Goal

Begin moving HIR-to-LIR struct layout lookup from rendered `TypeSpec::tag` /
`Module::struct_defs` strings toward `StructNameId` and structured
`LirStructDecl` lookup, while retaining legacy fallback.

## Why This Idea Exists

Idea 106 created `StructNameId -> LirStructDecl` as a shadow mirror. But many
HIR-to-LIR paths still look up layout, fields, GEP chains, byval handling, and
constant initializers through `TypeSpec::tag` and `mod.struct_defs.find(tag)`.

That is acceptable as a bridge, but the next solidification step is to make
HIR-to-LIR prefer `StructNameId` when the corresponding LIR struct declaration
already exists.

## Scope

Audit and update HIR-to-LIR paths around:

- struct size/alignment helpers used by lowering
- GEP and lvalue field-chain resolution
- const initializer aggregate/field lookup
- byval/byref aggregate call handling
- nested struct field lookup
- fallback paths that still rely on `TypeSpec::tag`

## Dual-Track Rules

- Prefer `StructNameId` / `LirStructDecl` lookup only where the ID is known and
  parity with legacy lookup can be observed.
- Keep `TypeSpec::tag` and `Module::struct_defs` fallback intact.
- Record mismatches between structured layout lookup and legacy tag lookup.
- Do not change emitted LLVM output or aggregate layout.
- Do not erase high-level HIR record semantics into LIR beyond lowered field
  layout.

## Acceptance Criteria

- Selected HIR-to-LIR layout lookup paths can use `StructNameId` first.
- Legacy `TypeSpec::tag` lookup remains available and behavior-preserving.
- Parity/mismatch observation exists for structured-vs-legacy layout lookup.
- Focused aggregate, field access, and initializer tests pass without output
  drift.
