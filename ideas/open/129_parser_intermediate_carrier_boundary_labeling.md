# Parser Intermediate Carrier Boundary Labeling

Status: Open
Created: 2026-04-29

Parent Ideas:
- [123_parser_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/123_parser_legacy_string_lookup_removal_convergence.md)
- [128_parser_ast_handoff_role_labeling.md](/workspaces/c4c/ideas/open/128_parser_ast_handoff_role_labeling.md)

## Goal

Clarify which parser-side helper structs and public `Parser` members are
parse-time-only state versus AST handoff producers.

## Why This Idea Exists

`Parser` exposes many members by project convention, and `parser_types.hpp`
contains carriers such as qualified-name refs, template parse results, namespace
contexts, symbol tables, and alias-template metadata. Some of these are purely
parser-local. Others are the source for fields later copied into `Node` or
`TypeSpec`.

Because public visibility does not equal cross-IR contract in this codebase,
the boundary should be documented directly in the parser facade and parser type
definitions before deeper cleanup work.

## Scope

- Review `src/frontend/parser/parser.hpp` and
  `src/frontend/parser/parser_types.hpp`.
- Group or comment the `Parser` public surface by role:
  - public parse entry/facade
  - parser-local state references
  - lookup/binding helpers
  - AST handoff producers/builders
  - diagnostics/debug/testing hooks
- Label parser helper structs as parse-time carrier, AST projection source, or
  cross-stage durable payload.
- Identify parser-local `std::string` use that is legitimate because it never
  crosses the AST/Sema/HIR boundary.
- Identify parser-side rendered spelling that does cross the boundary and should
  eventually become TextId / structured-table backed.

## Out Of Scope

- Changing member visibility.
- Moving helper structs or introducing new ownership.
- Replacing parser lookup behavior.
- Editing `Node` / `TypeSpec` beyond references needed for comments.

## Acceptance Criteria

- A reader can distinguish parser-local state from AST handoff producer state
  without tracing implementation files.
- `std::string` use in parser helper structs is classified as local-only,
  generated-name-only, or suspicious cross-stage authority.
- Suspicious cross-stage string use is captured in follow-up idea text rather
  than silently expanded into this documentation pass.
- No parser behavior changes.
