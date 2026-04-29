# Parser AST Template Payload String Bridge Cleanup

Status: Active
Source Idea: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md

## Purpose

Separate parser-produced AST and template display strings from semantic payload
authority at the parser-to-AST and AST-to-Sema/HIR boundary.

## Goal

Make parser AST/template payloads structured-primary for lookup,
substitution, deferred member-type decisions, and downstream ingress while
preserving rendered strings only as display, diagnostics, debug, generated
spelling, or explicitly quarantined compatibility bridges.

## Core Rule

Do not let parser-produced `std::string` AST/template payloads decide semantic
template, type, value, or member lookup when `TextId`, `QualifiedNameKey`,
`TemplateParamId`, direct symbol/record references, or existing structured
template argument fields are available.

## Read First

- `ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/expressions.cpp`
- `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/ast/ast.hpp`
- Sema/HIR ingress code that consumes parser-produced AST spelling
- `tests/frontend/frontend_parser_tests.cpp`

## Current Targets

- `ParserAliasTemplateInfo::param_names`
- `ParserTemplateArgParseResult::nttp_name`
- `$expr:` text copied into `template_arg_nttp_names`
- `TemplateArgRef::debug_text`
- `Node::name`
- `Node::unqualified_name`
- `Node::template_origin_name`
- `TypeSpec::tag`
- `TypeSpec::tpl_struct_origin`
- `TypeSpec::deferred_member_type_name`
- Sema/HIR consumers that reconstruct semantic identity from parser-produced
  AST spelling

## Non-Goals

- Do not remove AST display fields before downstream consumers are migrated.
- Do not replace every diagnostic, dump, or debug string.
- Do not rework final emitted symbol names, generated spelling, or mangling.
- Do not rewrite unrelated parser symbol-table, Sema, or HIR storage.
- Do not weaken existing template, deferred member-type, or AST dump tests.
- Do not add testcase-shaped string matching as a substitute for structured
  payload authority.

## Working Model

- Parser display strings are allowed at projection boundaries after structured
  identity has selected the entity or payload.
- Template and AST semantic authority should flow through existing structured
  carriers where available, or through newly exposed narrow carriers when a
  string field is the only current bridge.
- String payloads that must remain should be classified as display-only,
  diagnostic/debug-only, generated spelling, or explicit compatibility
  fallback.
- Fallback lookup is acceptable only where structured identity is genuinely
  absent, and the fallback must be named, narrow, and covered by tests.

## Execution Rules

- Keep source idea 134 unchanged unless durable source intent changes.
- Update `todo.md` for routine packet progress and proof.
- Prefer small, testable parser/Sema boundary slices over broad frontend
  rewrites.
- When retaining a string field or overload, make its role clear from naming,
  comments, or nearby tests.
- Add or update tests before claiming parser-produced spelling no longer acts
  as semantic authority.
- Escalate for review if a slice mostly changes expectations, weakens a test,
  or proves only one narrow template case while nearby same-feature cases
  remain unexamined.

## Step 1: Classify AST and Template Payload String Authority

Goal: produce a precise map of parser-produced AST/template strings that still
affect semantic lookup, substitution, or deferred member-type behavior.

Primary target:
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/expressions.cpp`
- `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/ast/ast.hpp`

Actions:
- Inspect every parser writer of `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`,
  `template_arg_nttp_names`, and `TemplateArgRef::debug_text`.
- Inspect parser writes to `Node::name`, `Node::unqualified_name`,
  `Node::template_origin_name`, `TypeSpec::tag`,
  `TypeSpec::tpl_struct_origin`, and
  `TypeSpec::deferred_member_type_name`.
- Trace the first Sema/HIR consumers that use those fields for lookup,
  substitution, or deferred member-type decisions.
- Classify each field as `structured-primary bridge`, `display-only`,
  `diagnostic/debug-only`, `generated spelling`, `fallback-only`, or
  `semantic string authority`.
- Record the first owned implementation packet in `todo.md`; do not edit
  implementation files in this classification step unless explicitly
  delegated.

Completion check:
- `todo.md` contains the classification map and names the first helper or
  payload family whose authority will be changed.
- No implementation files are changed by this step unless the executor packet
  is explicitly delegated.

## Step 2: Make Alias-Template Parameter Payloads Structured-Primary

Goal: prevent alias-template parameter substitution from relying on rendered
parameter names when structured template parameter identity is available.

Primary target:
- `ParserAliasTemplateInfo::param_names`
- alias-template parsing and projection in `declarations.cpp`
- downstream consumers of alias-template parameter payloads

Actions:
- Preserve rendered parameter names only as display or compatibility text.
- Prefer `TemplateParamId`, `TextId`, parser symbol ids, or existing
  structured template parameter records for substitution authority.
- Convert semantic consumers that can use structured parameter identity.
- Keep any string fallback narrow, named, and covered by tests.
- Add or update alias-template tests that would fail if rendered parameter
  spelling alone selected the semantic payload.

Completion check:
- Alias-template parameter tests cover structured substitution success and a
  spelling mismatch or duplicate-name case that cannot be passed by string
  authority alone.
- Narrow proof includes the focused parser/frontend test target for touched
  alias-template code.

## Step 3: Quarantine NTTP and Template Argument Debug Text

Goal: ensure NTTP names, `$expr:` text, and template argument debug text do not
act as semantic template argument identity.

Primary target:
- `ParserTemplateArgParseResult::nttp_name`
- `$expr:` text copied into `template_arg_nttp_names`
- `TemplateArgRef::debug_text`
- template argument parsing and projection code

Actions:
- Prefer structured template argument payloads for semantic lookup and
  substitution.
- Keep NTTP/debug strings only for display, diagnostics, dumps, or explicit
  compatibility fallback.
- Make any fallback branch visible in code and tests.
- Add or update tests for NTTP names and expression template arguments where
  rendered text could mask identity mismatch.

Completion check:
- NTTP/template argument tests prove structured payloads decide semantics and
  debug/display strings do not silently select a different entity.
- Remaining debug text users are classified as display, diagnostics, debug, or
  explicit fallback.

## Step 4: Tighten AST Boundary Fields and Deferred Member Types

Goal: stop parser-produced AST spelling from being the primary source for
semantic type/template-origin/deferred-member decisions when structured
carriers exist.

Primary target:
- `Node::name`
- `Node::unqualified_name`
- `Node::template_origin_name`
- `TypeSpec::tag`
- `TypeSpec::tpl_struct_origin`
- `TypeSpec::deferred_member_type_name`
- parser call sites in declarations, expressions, and type declarators
- first Sema/HIR ingress consumers for these fields

Actions:
- Convert semantic call sites to consume `TextId`, `QualifiedNameKey`,
  structured template payloads, or direct symbol/record references where
  available.
- Keep AST string fields only at final AST spelling, diagnostics, debug, dump,
  generated spelling, or explicit compatibility boundaries.
- Make deferred member-type fallback behavior narrow, named, and tested.
- Avoid changing final AST display spelling except where necessary to remove
  semantic reliance on compatibility spelling.

Completion check:
- Focused tests cover deferred member types, template-origin names, and AST
  boundary payloads where rendered spelling could previously mask identity
  mismatch.
- `rg` over touched parser/Sema/HIR code shows remaining string payload users
  are classified as display, diagnostics, debug, generated spelling, or
  fallback-only.

## Step 5: Validation and Acceptance

Goal: prove the cleanup did not regress parser/Sema/HIR behavior while removing
parser-produced AST/template strings as silent semantic authority.

Actions:
- Run the focused build or test target needed for touched parser/Sema/HIR code.
- Run `frontend_parser_tests`.
- Run targeted CTest selectors for alias templates, NTTP/template arguments,
  deferred member types, template-origin payloads, and relevant AST/Sema/HIR
  ingress cases.
- If implementation touches broad parser-to-Sema or Sema-to-HIR ingress
  behavior across several payload families, ask the supervisor to escalate to a
  broader frontend or full CTest pass.

Completion check:
- Fresh proof is recorded in `test_after.log` by the executor or in the
  supervisor-designated proof artifact.
- `todo.md` records the final packet status and any remaining fallback-only
  string payload paths.
- Acceptance criteria from the source idea are satisfied without testcase
  overfit.
