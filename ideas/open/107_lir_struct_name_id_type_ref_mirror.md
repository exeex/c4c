# LIR StructNameId TypeRef Mirror

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [106_shared_struct_name_table_for_lir_type_identity.md](/workspaces/c4c/ideas/closed/106_shared_struct_name_table_for_lir_type_identity.md)

## Goal

Extend the `StructNameId` dual-track strategy from struct declarations into
LIR type references.

The first slice should make struct-valued `LirTypeRef` instances capable of
carrying a `StructNameId` mirror beside their existing rendered LLVM type
string, without changing printer output or removing legacy text.

## Why This Idea Exists

Idea 106 introduced the shared `StructNameId` / `StructNameTable` and a
shadow-only `LirStructDecl` mirror beside rendered `type_decls`. That proves
struct declarations can be dual-written and parity checked.

The next bottleneck is that most LIR operation fields still carry type identity
as rendered text through `LirTypeRef`, `llvm_ty`, `llvm_alloca_ty`, and related
helpers. For struct types, those rendered strings should gain a cheap
`StructNameId` mirror so later cleanup can stop using `%struct.X` strings as
semantic type identity.

## Scope

Audit and update:

- `src/codegen/lir/types.hpp`
- `LirTypeRef` construction and verification
- HIR-to-LIR helpers that create struct LLVM type strings
- shared LLVM helper surfaces that render `TypeSpec::tag` into `%struct.*`
- LIR verifier checks that can observe structured struct-type parity

## Dual-Track Rules

- Keep the rendered `LirTypeRef` string as the printer authority.
- Add `StructNameId` only as a structured mirror when the type is a struct or
  union reference and the ID is known.
- Shadow-render `StructNameId` through `StructNameTable` and compare it with
  the legacy rendered type string.
- Preserve existing behavior when no `StructNameId` is available.
- Do not remove `TypeSpec::tag`, `llvm_ty`, `llvm_alloca_ty`, or rendered type
  strings in this idea.

## Suggested Execution Decomposition

1. Add optional `StructNameId` metadata to `LirTypeRef` or a narrow companion
   wrapper that matches local style.
2. Add constructors/factories for struct type refs that dual-write rendered
   text and `StructNameId`.
3. Thread `StructNameId` through HIR-to-LIR type-ref creation where the struct
   name has already been interned by idea 106.
4. Add verifier parity checks for struct type refs where both rendered text and
   `StructNameId` are present.
5. Keep printer output unchanged.

## Acceptance Criteria

- Struct/union LIR type refs can carry `StructNameId` beside rendered text.
- Existing LIR type strings remain present and authoritative for printing.
- Verifier or focused tests prove `StructNameId` rendering matches legacy type
  strings where both exist.
- No legacy rendered type string path is removed.
- Existing LLVM output remains unchanged.
