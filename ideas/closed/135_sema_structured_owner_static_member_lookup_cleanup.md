# Sema Structured Owner Static Member Lookup Cleanup

Status: Closed
Created: 2026-04-29
Closed: 2026-04-30

Parent Ideas:
- [130_sema_hir_ast_ingress_boundary_audit.md](/workspaces/c4c/ideas/open/130_sema_hir_ast_ingress_boundary_audit.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)
- [134_parser_ast_template_payload_string_bridge_cleanup.md](/workspaces/c4c/ideas/closed/134_parser_ast_template_payload_string_bridge_cleanup.md)

## Goal

Make Sema owner and static-member lookup use structured AST/parser identity
instead of rendered method, record, or member spelling wherever a structured
carrier is available.

## Why This Idea Exists

Step 1 of idea 130 found Sema paths where rendered spelling still appears to
decide semantic ownership after `SemaStructuredNameKey`, namespace context, or
AST unqualified-name fields exist nearby. These paths should be cleaned as Sema
consumer work rather than folded into the ingress audit or the parser payload
cleanup.

Suspicious Sema paths:

- `resolve_owner_in_namespace_context`
- `enclosing_method_owner_struct`
- `lookup_struct_static_member_type`
- unqualified variable lookup fallback that can recover from `n->name` when
  `n->unqualified_name` should be the structured AST spelling authority

## Scope

- Trace record-owner, method-owner, and static-member lookup from parser AST
  fields through Sema validation.
- Prefer `SemaStructuredNameKey`, namespace context ids, AST `TextId` fields,
  and direct declaration/type links over rendered owner/member strings.
- Keep rendered names only for diagnostics, display, debug output, or explicit
  compatibility fallback with structured-primary behavior.
- Coordinate with idea 134 for parser-produced AST fields such as `Node::name`
  and `Node::unqualified_name`, but keep Sema lookup-consumer changes here.
- Add focused Sema/frontend tests around namespace-qualified methods, static
  members, and unqualified variable/member lookup where rendered spelling could
  previously mask identity mismatches.

## Out Of Scope

- Rewriting parser name production.
- Redesigning `ResolvedTypeTable`.
- Removing diagnostic or display spelling from AST nodes.
- Broad `std::string` cleanup outside Sema owner/member lookup authority.

## Acceptance Criteria

- Sema owner and static-member lookup is structured-primary when parser AST
  identity is available.
- Any remaining rendered-spelling path is documented or guarded as
  compatibility-only, not silent semantic authority.
- Nearby same-feature tests cover namespace-qualified owner lookup, static
  member lookup, and unqualified-name fallback behavior.

## Closure Notes

Accepted as complete after Step 6 validation.

Implementation commits for this idea made Sema namespace-owner resolution,
enclosing method-owner lookup, static-member lookup, and unqualified lookup
prefer structured identity when parser AST carriers are present. Focused tests
cover namespace-qualified owner lookup, static members, method-owner context,
and unqualified-name fallback behavior where rendered spelling could previously
mask mismatches.

Remaining rendered-name paths are compatibility-only or non-semantic output
paths:

- rendered owner resolution remains a fallback when structured owner keys are
  absent or invalid
- static-member lookup falls back to legacy rendered maps only when no usable
  member `TextId`/record key is available
- unqualified `Node::name` lookup retains a narrow compatibility recovery via
  `Node::unqualified_name` for producers that still expose visible namespace
  spelling before Sema has bound locals
- diagnostics, display, debug output, and legacy string-facing bridges may
  continue to carry rendered spelling without becoming semantic authority

Close-time proof:

`cmake --build build --target c4cll frontend_parser_tests frontend_hir_lookup_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_lookup_tests|.*sema.*|cpp_negative_tests_bad_scope.*|negative_tests_bad_scope.*)$' >> test_after.log 2>&1`

Result: 1000/1000 tests passed.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS with 1000 passed before and 1000 passed after.
