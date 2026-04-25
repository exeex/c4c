# LIR Type Text Authority Demotion

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [112_lir_backend_legacy_type_surface_readiness_audit.md](/workspaces/c4c/ideas/closed/112_lir_backend_legacy_type_surface_readiness_audit.md)
- [113_backend_struct_decl_layout_table_dual_path.md](/workspaces/c4c/ideas/closed/113_backend_struct_decl_layout_table_dual_path.md)

## Goal

Demote selected LIR rendered type text fields from semantic authority to
printer-only or proof-only status once idea 112 and backend dual-path work show
they are safe.

## Why This Idea Exists

After `StructNameId` reaches declarations, type refs, signatures, globals,
externs, calls, and backend layout tables, some legacy text fields should no
longer be semantic lookup authority. They may still be needed as emitted text,
debug text, or compatibility proof, but not as the source of type identity.

This idea should clean only the surfaces classified as safe by idea 112 and any
BIR/backend follow-up proof. It should not attempt a big-bang removal of all
LIR type strings.

## Scope

Use idea 112 as the source of truth. Candidate fields include:

- `LirGlobal::llvm_type`
- `LirExternDecl::return_type_str`
- selected `LirFunction::signature_text` authority
- selected call return/argument formatted type text
- selected raw `LirTypeRef` text-only comparisons
- verifier checks that can become structured-first with text fallback

## Execution Rules

- Only demote paths classified as `safe-to-demote` or `legacy-proof-only`.
- Keep printer-only text for final LLVM/backend emission.
- Keep compatibility text where backend or verifier proof still requires it.
- Do not remove text fields that are still `backend-blocked`,
  `layout-authority-blocked`, or `type-ref-authority-blocked`.
- Do not treat `src/backend/mir/` legacy uses as cleanup targets or blockers;
  if they block compilation, exclude the relevant `.cpp` files from the
  current compile target.
- Preserve exact emitted output.

## Acceptance Criteria

- Selected LIR type text fields no longer act as semantic authority.
- Structured type refs or `StructNameId` are primary for the cleaned paths.
- Legacy text remains where it is printer-only, bridge-required, or proof-only.
- Focused plus broader validation passes without expectation downgrades.
