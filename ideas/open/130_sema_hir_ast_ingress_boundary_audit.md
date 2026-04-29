# Sema HIR AST Ingress Boundary Audit

Status: Open
Created: 2026-04-29

Parent Ideas:
- [124_hir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/124_hir_legacy_string_lookup_removal_convergence.md)
- [128_parser_ast_handoff_role_labeling.md](/workspaces/c4c/ideas/open/128_parser_ast_handoff_role_labeling.md)

## Goal

Mark and review the Sema/HIR ingress points that consume Parser AST fields so
the authoritative cross-stage interface is explicit.

## Why This Idea Exists

Sema already has structured-name helpers derived from AST `TextId` fields, and
Sema also produces `ResolvedTypeTable` keyed by `const Node*`. HIR lowers the AST
into its own module tables, but some helpers still rebuild TextIds from rendered
AST spelling while carrying compatibility strings in parallel.

The ingress boundary should say which AST fields are authority, which Sema
outputs are authority, and which string-based paths are display or compatibility
fallbacks.

## Scope

- Review Sema ingress helpers in `src/frontend/sema/validate.hpp`,
  `src/frontend/sema/validate.cpp`, and `src/frontend/sema/canonical_symbol.*`.
- Review HIR AST ingress helpers in `src/frontend/hir/hir.hpp`,
  `src/frontend/hir/hir_lowering_core.cpp`, `src/frontend/hir/hir_build.cpp`,
  and related `impl/` lowering helpers as needed.
- Label or document the role of:
  - `SemaStructuredNameKey`
  - `ResolvedTypeTable`
  - HIR `NamespaceQualifier` construction from AST fields
  - HIR name/TextId creation from AST spelling
- Audit `std::string` and raw spelling use at this ingress boundary.
- Classify legitimate string creation such as mangling, anonymous-name
  generation, section labels, diagnostics, or final display text separately
  from cross-stage semantic identity.

## Out Of Scope

- Replacing HIR lookup logic.
- Redesigning `ResolvedTypeTable`.
- Changing AST lifetime or ownership.
- Removing compatibility string fields from HIR.

## Acceptance Criteria

- The Sema/HIR AST ingress comments or notes clearly identify authority:
  AST TextId/structured fields, Sema resolved types, HIR module tables, or
  compatibility spelling.
- Any HIR ingress path that treats `std::string` or rendered spelling as
  cross-stage semantic authority is listed for a follow-up cleanup idea.
- Legitimate generated names are explicitly classified as non-cross-IR
  behavior.
- Existing frontend/HIR tests remain behaviorally unchanged.
