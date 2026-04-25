# LIR StructNameId For Globals Functions And Externs

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [107_lir_struct_name_id_type_ref_mirror.md](/workspaces/c4c/ideas/open/107_lir_struct_name_id_type_ref_mirror.md)

## Goal

Use the `StructNameId`-aware type-ref mirror in LIR global declarations,
function signatures, call return/argument typing, and extern declarations while
preserving all rendered signature text.

## Why This Idea Exists

After `StructNameId` reaches `LirTypeRef`, the next step is to make the major
HIR-to-LIR signature surfaces dual-write those structured type refs. Today
several fields still mix type identity and printer text:

- `LirGlobal::llvm_type`
- `LirFunction::signature_text`
- `LirExternDecl::return_type_str`
- call return and argument type strings
- helper-generated function type suffixes

This idea keeps those strings as compatibility/printer text, but makes
struct-bearing type surfaces carry structured mirrors for parity and later
cleanup.

## Scope

Audit and update:

- global lowering in HIR-to-LIR
- function definition/declaration lowering
- extern declaration dedup/output
- call lowering fields that carry return or argument type strings
- LIR verifier coverage for structured type refs in these surfaces

## Dual-Track Rules

- Keep rendered `signature_text`, `llvm_type`, and `return_type_str`.
- Add structured type refs beside those fields where the type is already known.
- Prefer exact shadow-render parity over behavior changes.
- Do not switch printer authority in this idea.
- Preserve `LinkNameId` as the authority for function/global symbol identity;
  `StructNameId` is only for struct type spelling/layout identity.

## Acceptance Criteria

- Globals, functions, calls, and extern declarations can carry struct-aware
  type refs beside rendered text.
- Existing printer output remains unchanged.
- Parity checks cover struct-bearing signatures and global declarations.
- Legacy text fields remain in place for compatibility.
