# Parser Namespace Visible Name Compatibility Spelling Cleanup

Status: Active
Source Idea: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md

## Purpose

Retire parser namespace and visible-name lookup paths where rendered
compatibility spelling can still act as semantic authority after structured
name identity is available.

## Goal

Make parser namespace lookup, visible-name resolution, and using-value alias
handling structured-primary while preserving rendered spelling only as a
display, diagnostic, debug, or explicitly quarantined compatibility bridge.

## Core Rule

Do not let `std::string` compatibility spelling silently decide namespace,
using-alias, type, value, or concept resolution when `QualifiedNameKey`,
`TextId`, parser symbol ids, or namespace context ids are available.

## Read First

- `ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/support.cpp`
- `tests/frontend/frontend_parser_tests.cpp`
- `tests/cpp/internal/postive_case/parse/using_namespace_directive_parse.cpp`
- `tests/cpp/internal/postive_case/parse/local_value_shadows_using_alias_assign_expr_parse.cpp`

## Current Targets

- `Parser::VisibleNameResult::compatibility_spelling`
- `ParserNamespaceState::UsingValueAlias::compatibility_name`
- `Parser::compatibility_namespace_name_in_context`
- `Parser::lookup_using_value_alias` result and string overloads
- `Parser::lookup_value_in_context`, `lookup_type_in_context`, and
  `lookup_concept_in_context` result and string overloads
- `Parser::resolve_visible_value`, `resolve_visible_type`,
  `resolve_visible_concept`, and their string-returning wrappers
- AST projection sites that still need final rendered names, especially
  `declarations.cpp`, `expressions.cpp`, and type declarator paths

## Non-Goals

- Do not change source-language namespace or using-declaration semantics.
- Do not remove final display names from AST nodes, diagnostics, parser debug
  output, or transitional AST projection.
- Do not rewrite unrelated parser symbol-table storage.
- Do not add testcase-shaped shortcuts or named-case compatibility gates.
- Do not weaken existing tests or mark supported nearby cases unsupported.

## Working Model

- Structured lookup authority is `QualifiedNameKey`, `TextId`, parser symbol
  ids, and namespace context ids.
- Rendered spelling is allowed after structured lookup has selected an entity.
- `VisibleNameResult` should carry enough structured data for callers to avoid
  re-looking up by string.
- String-returning helpers are compatibility/projection bridges. They may
  remain only when their names and implementation make the structured-primary
  path explicit.
- Fallback lookup is acceptable only where a structured id is genuinely absent,
  and the fallback must be visible and tested as fallback behavior.

## Execution Rules

- Keep source idea 133 unchanged unless durable source intent changes.
- Update `todo.md` for routine packet progress and proof.
- Prefer small, testable parser slices over broad parser rewrites.
- When keeping a compatibility field or overload, document whether it is
  display-only, diagnostic-only, debug-only, or fallback-only.
- Add or update tests before claiming that rendered spelling can no longer
  decide semantic lookup.
- Escalate for review if a slice mostly changes expectations, weakens a test,
  or proves only one narrow case while nearby same-feature cases remain
  unexamined.

## Step 1: Classify Compatibility Spelling Authority

Goal: produce a precise map of where compatibility spelling still affects
semantic resolution.

Primary target:
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/parser_state.hpp`

Actions:
- Inspect every writer and reader of `VisibleNameResult::compatibility_spelling`.
- Inspect `UsingValueAlias::compatibility_name` construction, mutation, and
  lookup in `declarations.cpp`, `support.cpp`, and `core.cpp`.
- Classify each string overload of namespace and visible-name helpers as one of
  `structured-primary bridge`, `display-only bridge`, `fallback-only`, or
  `semantic string authority`.
- Record the classification and the first implementation packet in `todo.md`;
  do not edit the source idea for routine findings.

Completion check:
- `todo.md` identifies the first owned code packet and names the specific
  helper family whose authority will be changed first.
- No implementation files are changed by this step unless the executor packet
  is explicitly delegated.

## Step 2: Make Using-Value Alias Lookup Structured-Primary

Goal: ensure using-value aliases resolve from `QualifiedNameKey` first and use
compatibility spelling only as a display or explicit fallback.

Primary target:
- `ParserNamespaceState::UsingValueAlias`
- `Parser::lookup_using_value_alias`
- `Parser::resolve_visible_value`
- `Parser::resolve_visible_type`
- parser testing hooks in `support.cpp`

Actions:
- Preserve `UsingValueAlias::target_key` as the semantic lookup authority.
- Quarantine `compatibility_name` so it cannot make a missing or mismatched
  structured target look semantically valid unless the path is explicitly
  fallback-only.
- Adjust string overloads to delegate through the structured result overload
  and to return rendered spelling only after structured resolution succeeds.
- Add tests that mutate compatibility spelling and prove the structured target
  still decides the resolved value/type behavior.

Completion check:
- Using-value alias tests cover structured target success, compatibility
  spelling mismatch, and local shadowing or namespace-scope interaction.
- Narrow proof includes `frontend_parser_tests` and at least one parse-only
  using-alias case.

## Step 3: Tighten Namespace and Visible-Name Context Lookup

Goal: remove or quarantine paths where rendered namespace text decides lookup
when namespace context ids and text ids are available.

Primary target:
- `Parser::compatibility_namespace_name_in_context`
- `Parser::bridge_name_in_context`
- `Parser::lookup_value_in_context`
- `Parser::lookup_type_in_context`
- `Parser::lookup_concept_in_context`
- `Parser::resolve_namespace_context`
- `Parser::resolve_namespace_name`

Actions:
- Prefer `find_named_namespace_child`, namespace context ids, and
  `qualified_key_in_context` over rendered namespace strings.
- Keep `compatibility_namespace_name_in_context` only as a named compatibility
  renderer, not a semantic lookup source.
- Replace semantic string fallback checks with structured lookup where the
  required `TextId` or `QualifiedNameKey` exists.
- Make any remaining fallback branch narrow, named, and covered by a test.

Completion check:
- Qualified namespace and using-namespace tests prove structured lookup wins
  when rendered spelling could mask identity mismatch.
- Existing namespace parse-only and frontend parser tests remain green.

## Step 4: Quarantine String Overloads and AST Projection Bridges

Goal: make string-returning visible-name and namespace helpers clearly
projection-only or remove them after structured call sites have equivalents.

Primary target:
- `resolve_visible_*` string overloads and `resolve_visible_*_name`
- `lookup_*_in_context` string overloads
- call sites in `declarations.cpp`, `expressions.cpp`, and
  `impl/types/declarator.cpp`

Actions:
- Convert semantic call sites to consume `VisibleNameResult`,
  `QualifiedNameKey`, `TextId`, or namespace context ids directly.
- Keep string-returning helpers only at final AST spelling, diagnostic, debug,
  or explicit bridge boundaries.
- Rename or comment remaining compatibility helpers if their role is not clear
  from the existing local naming.
- Avoid changing AST output spelling except where the source idea explicitly
  requires removing semantic reliance on compatibility spelling.

Completion check:
- `rg` over parser sources shows remaining compatibility spelling users are
  classified as display, diagnostics, debug, AST projection, or fallback-only.
- Narrow tests prove visible type/value/concept resolution does not depend on a
  corrupted compatibility spelling.

## Step 5: Validation and Acceptance

Goal: prove the cleanup did not regress parser behavior while removing
compatibility spelling as semantic authority.

Actions:
- Run the focused build or test target needed for touched parser code.
- Run `frontend_parser_tests`.
- Run targeted parse-only CTest selectors for using aliases, namespace
  directives, qualified namespace lookup, and visible-name lookup.
- If implementation touches broad parser lookup behavior across several
  helper families, ask the supervisor to escalate to a broader parser or full
  CTest pass.

Completion check:
- Fresh proof is recorded in `test_after.log` by the executor or in the
  supervisor-designated proof artifact.
- `todo.md` records the final packet status and any remaining fallback-only
  compatibility paths.
- Acceptance criteria from the source idea are satisfied without testcase
  overfit.
