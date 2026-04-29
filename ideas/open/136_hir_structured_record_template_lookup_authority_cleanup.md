# HIR Structured Record Template Lookup Authority Cleanup

Status: Open
Created: 2026-04-29

Parent Ideas:
- [130_sema_hir_ast_ingress_boundary_audit.md](/workspaces/c4c/ideas/open/130_sema_hir_ast_ingress_boundary_audit.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)
- [134_parser_ast_template_payload_string_bridge_cleanup.md](/workspaces/c4c/ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md)

## Goal

Promote HIR structured owner, member, method, and template lookup keys from
parity mirrors to semantic authority where rendered names still decide lookup.

## Why This Idea Exists

Step 2 of idea 130 found HIR paths that already build structured carriers such
as `NamespaceQualifier`, `TextId`, `ModuleDeclLookupKey`, and
`HirRecordOwnerKey`, but still return or derive semantic lookup results from
rendered function, record, template, or member strings. The cleanup should be a
focused HIR implementation idea, not part of the ingress audit.

Suspicious HIR paths:

- `parse_scoped_function_name`
- out-of-class method ownership derived from rendered `fn->name` or `n->name`
- scoped static-member lookup derived from rendered names
- rendered template struct primary and specialization lookup, including
  owner-parity-only checks
- struct method lookup, static-member lookup, and member-symbol lookup where
  rendered tag/member maps are returned while owner-key maps only check parity

## Scope

- Trace HIR lowering and lookup paths for out-of-class methods, scoped static
  members, template struct primaries/specializations, struct methods, static
  members, and member symbols.
- Prefer `NamespaceQualifier`, declaration `TextId`, `ModuleDeclLookupKey`,
  `HirRecordOwnerKey`, and existing by-owner maps as lookup authority.
- Convert rendered maps to compatibility mirrors, diagnostics, debug output, or
  temporary fallback paths with mismatch counters or explicit assertions.
- Coordinate with idea 134 where parser-produced AST spelling still reaches HIR
  as payload text, but keep HIR lookup-table authority changes here.
- Add focused HIR/frontend tests for namespace-qualified records, out-of-class
  methods, scoped static members, template primary/specialization lookup, and
  member-symbol lookup under owners with colliding rendered names.

## Out Of Scope

- Removing final mangled/link names or generated specialization names.
- Replacing dump, diagnostic, label, asm, or display strings.
- Rewriting parser AST production.
- Bulk deleting HIR compatibility string fields before consumers are migrated.

## Acceptance Criteria

- HIR record, member, method, static-member, and template lookup paths are
  structured-primary where structured keys already exist.
- Rendered lookup maps are removed, demoted to compatibility-only, or guarded
  so they cannot silently decide semantic resolution against structured keys.
- Tests cover nearby same-feature cases rather than only one observed
  suspicious call path.
