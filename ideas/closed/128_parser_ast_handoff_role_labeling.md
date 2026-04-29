# Parser AST Handoff Role Labeling

Status: Closed
Created: 2026-04-29
Closed: 2026-04-29

Parent Ideas:
- [123_parser_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/123_parser_legacy_string_lookup_removal_convergence.md)

## Goal

Make the Parser AST carrier fields self-documenting by labeling which fields are
cross-stage handoff contract, parser-produced semantic hints, compatibility
spelling, or parser recovery/debug payload.

## Why This Idea Exists

The parser now emits a mixed AST surface consumed by Sema and HIR. `Node` and
`TypeSpec` already have rough grouping comments, but the grouping mostly follows
field shape rather than boundary role. That makes it hard to tell which payload
is intentionally passed across IR stages and which payload is a legacy bridge or
parser convenience.

Before changing lookup logic or moving data into new carriers, the handoff
surface should be labeled in-place so future cleanup can distinguish structure
from behavior.

## Scope

- Review `src/frontend/parser/ast.hpp`, especially `TypeSpec`, `TemplateArgRef`,
  and `Node`.
- Reorder fields only when the move is comment-only / layout-neutral for
  behavior and keeps related handoff payload together.
- Add section comments that classify fields by role:
  - cross-stage contract
  - parser-produced semantic hint
  - compatibility/display spelling
  - recovery/debug placeholder
  - legacy bridge, not preferred authority
- Keep the flat AST carrier model intact.
- Review raw spelling and `std::string`-derived handoff usage called out by the
  comments, but do not replace behavior in this idea.

## Out Of Scope

- Moving fields out of `Node` or `TypeSpec`.
- Introducing access-control labels or encapsulation.
- Changing parser, Sema, HIR, or codegen behavior.
- Replacing string lookup paths with TextId paths.

## Acceptance Criteria

- `Node` and `TypeSpec` comments clearly identify which payload is intended to
  be consumed after parsing.
- Fields that remain as compatibility/display spelling are labeled as such.
- Any suspicious cross-stage `std::string` or rendered-spelling authority found
  during review is listed in a follow-up idea instead of being fixed ad hoc.
- Parser and frontend tests still build or compile with no behavior change.

## Completion Summary

The active runbook labeled `TypeSpec`, `TemplateArgRef`, and `Node` parser AST
handoff fields by boundary role while preserving the flat carrier model and
behavior. Suspicious string-authority cleanup was left to follow-up open ideas,
including `ideas/open/130_sema_hir_ast_ingress_boundary_audit.md` and
`ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md`.

Close proof used matching frontend parser regression logs:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
The regression guard passed with 1 passed / 0 failed before and after.
