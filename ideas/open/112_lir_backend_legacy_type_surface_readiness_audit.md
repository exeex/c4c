# LIR Backend Legacy Type Surface Readiness Audit

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [107_lir_struct_name_id_type_ref_mirror.md](/workspaces/c4c/ideas/closed/107_lir_struct_name_id_type_ref_mirror.md)
- [108_lir_struct_name_id_for_globals_functions_and_externs.md](/workspaces/c4c/ideas/closed/108_lir_struct_name_id_for_globals_functions_and_externs.md)
- [109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md](/workspaces/c4c/ideas/closed/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md)
- [110_lir_struct_type_printer_authority_readiness_audit.md](/workspaces/c4c/ideas/closed/110_lir_struct_type_printer_authority_readiness_audit.md)
- [111_lir_struct_decl_printer_authority_switch.md](/workspaces/c4c/ideas/closed/111_lir_struct_decl_printer_authority_switch.md)

## Goal

Audit remaining LIR/backend legacy type surfaces after the struct declaration
printer authority switch and classify which legacy APIs or rendered text fields
can be demoted, which remain proof-only, and which are blocked by backend,
layout, or type-ref authority.

This idea is a report-only checkpoint. It should not perform implementation
cleanup.

## Why This Idea Exists

Ideas 106 through 111 moved LLVM struct declarations onto the structured
`StructNameId` / `LirStructDecl` path while preserving dual-track proof. The
printer no longer needs `type_decls` as authority for struct declaration
emission, but many other surfaces still consume legacy text:

- BIR backend aggregate layout still consumes `module.type_decls`.
- `src/backend/mir/` is planned for a full rewrite and must not be treated as a
  blocker for this cleanup route. If MIR `.cpp` files block the route, the
  intended response is to remove those files from the current compile target,
  not to migrate or preserve their legacy type-surface usage.
- HIR-to-LIR selected layout lookup still keeps `StructuredLayoutLookup::legacy_decl`.
- `LirGlobal::llvm_type`, `LirFunction::signature_text`, and
  `LirExternDecl::return_type_str` remain emitted text authority.
- Raw `LirTypeRef` equality/output and many operation fields still rely on text.

Before another cleanup pass, the project needs a precise blocker map and
ownership split.

## Audit Scope

Inspect at least:

- `LirModule::type_decls`
- `LirModule::struct_decls`
- `LirStructuredLayoutObservation`
- `StructuredLayoutLookup::legacy_decl`
- `LirGlobal::llvm_type` and `llvm_type_ref`
- `LirFunction::signature_text` and signature mirrors
- `LirExternDecl::return_type_str` and `return_type`
- call return/argument type refs and formatted call text
- `LirTypeRef` equality/output and raw string-only construction paths
- BIR layout consumers under `src/backend/bir/lir_to_bir/memory/`
- MIR/aarch64 legacy consumers only to label them as `planned-rebuild`; do not
  propose migration work under `src/backend/mir/`, and record any compile
  blockage as a compile-target exclusion task rather than a semantic blocker
- backend helpers that parse type text or scan `module.type_decls`

## Classification

Classify each remaining legacy surface as one of:

- `safe-to-demote`: structured authority and proof are complete enough.
- `legacy-proof-only`: retained only for mismatch/backcompat observation.
- `backend-blocked`: BIR or other active backend consumers still require text.
- `planned-rebuild`: legacy surface lives under a subsystem scheduled for full
  replacement; it is not a cleanup blocker. If it blocks compilation, exclude
  the relevant `.cpp` from the compile target instead of migrating the code.
- `layout-authority-blocked`: HIR-to-LIR still consumes legacy layout facts.
- `type-ref-authority-blocked`: raw `LirTypeRef` text remains authority.
- `printer-only`: final emitted text only.
- `bridge-required`: still needed as an explicit compatibility handoff.
- `needs-more-parity-proof`: structured mirror exists but proof is too narrow.

## Required Output

Write a review artifact under `review/`, for example:

```text
review/112_lir_backend_legacy_type_surface_readiness_audit.md
```

The report should include:

- remaining legacy API/text field table
- blocker owner for each blocked surface
- recommended scope for ideas 113, 114, and 115
- proof gaps and recommended validation subsets
- explicit statement of which legacy paths must not be removed yet
- explicit statement that `src/backend/mir/` is not a current migration target
  and must not block BIR/LIR cleanup; if it blocks compilation, identify the
  compile-target exclusion instead of proposing MIR migration

## Acceptance Criteria

- Remaining LIR/backend legacy type surfaces are classified precisely.
- BIR/backend, planned-MIR-rebuild exclusions, layout-authority, and type-ref
  blockers are separated.
- Safe demotion candidates are separated from proof-only and bridge-required
  paths.
- No implementation cleanup occurs in this audit.
- Follow-up ideas can start without re-deriving blocker ownership.
