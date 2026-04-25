Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser/Sema Leftover Callers

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and `todo.md` for idea 98. No executor
packet has run yet.

## Suggested Next

Start Step 1 by inventorying the parser/sema leftovers from
`review/97_structured_identity_completion_audit.md`, separating parser/sema-owned
structured-input opportunities from bridge-required string-only callers.

## Watchouts

- Keep this plan limited to parser/sema cleanup; HIR module lookup migration
  belongs to idea 99.
- Preserve rendered-string bridges required by AST, HIR, consteval, diagnostics,
  codegen, and link-name output.
- Do not touch parser struct/tag maps, template rendered names, `TypeSpec::tag`
  outputs, or HIR/type/codegen identity surfaces.
- Do not downgrade expectations or add testcase-shaped exceptions.

## Proof

No code proof required for lifecycle-only activation. The first implementation
packet should at minimum run `cmake --build --preset default` plus the focused
frontend/parser/sema subset selected by the supervisor.
