# LIR Struct Type Printer Authority Readiness Audit

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [106_shared_struct_name_table_for_lir_type_identity.md](/workspaces/c4c/ideas/closed/106_shared_struct_name_table_for_lir_type_identity.md)
- [107_lir_struct_name_id_type_ref_mirror.md](/workspaces/c4c/ideas/open/107_lir_struct_name_id_type_ref_mirror.md)
- [108_lir_struct_name_id_for_globals_functions_and_externs.md](/workspaces/c4c/ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md)
- [109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md](/workspaces/c4c/ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md)

## Goal

Audit whether the `StructNameId` structured path is ready to become printer
authority for LLVM struct type declarations and selected struct type
references.

This is a readiness checkpoint, not a broad cleanup implementation.

## Why This Idea Exists

Idea 106 intentionally kept rendered `type_decls` as the printer authority.
Ideas 107 through 109 should expand the structured mirror into type refs,
signature/global surfaces, and selected layout lookup. Before any legacy
rendered type text is demoted, the project needs a focused audit of parity
coverage, remaining string-only surfaces, and validation stability.

## Audit Scope

Inspect at least:

- `LirModule::type_decls`
- `LirModule::struct_decls`
- `LirTypeRef` struct-name mirrors
- global/function/extern signature text fields
- HIR-to-LIR layout lookup paths still using `TypeSpec::tag`
- LIR verifier shadow-render/parity checks
- emitted LLVM diffs from focused struct/template/global-init coverage

## Classification

Classify remaining rendered struct/type strings as:

- `printer-authority-ready`: structured render has exact parity and enough
  proof.
- `legacy-proof-only`: retained only for mismatch observation.
- `bridge-required`: downstream still lacks structured identity.
- `printer-only`: final text output only.
- `blocked-by-type-ref`: waiting for more structured `LirTypeRef` coverage.
- `blocked-by-layout-lookup`: waiting for more `StructNameId` layout lookup.

## Required Output

Write a review artifact under `review/`, for example:

```text
review/110_lir_struct_type_printer_authority_readiness_audit.md
```

The report should recommend whether to create a follow-up idea to switch
printer authority from `type_decls` to `struct_decls`, or whether more dual-path
coverage is needed first.

## Acceptance Criteria

- Struct/type rendered-string surfaces are classified precisely.
- Printer authority switch candidates are separated from bridge-required text.
- Remaining blockers are tied to concrete files and tests.
- No legacy removal is performed by this audit.
