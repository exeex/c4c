# Parser Namespace Visible Name Compatibility Spelling Cleanup

Status: Open
Created: 2026-04-29

Parent Ideas:
- [129_parser_intermediate_carrier_boundary_labeling.md](/workspaces/c4c/ideas/open/129_parser_intermediate_carrier_boundary_labeling.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Retire parser namespace and visible-name lookup paths where compatibility
spelling can still act as semantic authority after structured name identity is
available.

## Why This Idea Exists

Step 3 of idea 129 found namespace and visible-name helpers that carry rendered
spelling next to `QualifiedNameKey` and `TextId` identity. The compatibility
spelling is useful for diagnostics and transitional AST projection, but it
should not be the primary key for semantic namespace or using-alias resolution.

Suspicious paths:

- `Parser::VisibleNameResult::compatibility_spelling`
- `ParserNamespaceState::UsingValueAlias::compatibility_name`
- `compatibility_namespace_name_in_context`
- string overloads of namespace and visible-name lookup helpers

## Scope

- Trace namespace lookup, visible-name resolution, using-value alias handling,
  and AST name projection sites that accept rendered strings.
- Prefer `QualifiedNameKey`, `TextId`, and parser symbol ids as lookup
  authority.
- Keep compatibility spelling only for display, diagnostics, parser debug
  output, or temporary bridges with explicit structured-primary behavior.
- Remove or quarantine string overloads once call sites have structured
  equivalents.

## Out Of Scope

- Changing source-language namespace semantics.
- Removing final display names from AST nodes or diagnostics.
- Rewriting unrelated parser symbol-table storage.

## Acceptance Criteria

- Namespace and visible-name semantic lookup has structured-primary call paths.
- Any remaining compatibility spelling fields are documented as display or
  fallback-only and cannot silently decide semantic resolution.
- Tests cover using aliases, qualified namespaces, and visible-name lookup where
  rendered spelling could previously mask identity mismatches.
