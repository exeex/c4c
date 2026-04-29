Status: Active
Source Idea Path: ideas/open/128_parser_ast_handoff_role_labeling.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current AST Carrier Layout

# Current Packet

## Just Finished

Lifecycle activation created this active plan and initialized execution state.

## Suggested Next

Delegate Step 1 to an executor: inspect `src/frontend/parser/ast.hpp` and map
the current `TypeSpec`, `TemplateArgRef`, and `Node` field groups before making
comment-only labeling edits.

## Watchouts

- Keep this runbook behavior-preserving.
- Do not replace string lookup paths or change parser/Sema/HIR/codegen behavior.
- Record suspicious cross-stage string authority as follow-up idea work instead
  of fixing it ad hoc.

## Proof

Lifecycle-only activation; no build or test proof required.
