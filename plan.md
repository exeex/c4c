# Parser Visible Name Resolution Structured Result

Status: Active
Source Idea: ideas/open/87_parser_visible_name_resolution_structured_result.md
Activated from: ideas/open/87_parser_visible_name_resolution_structured_result.md

## Purpose

Move parser visible-name resolution off rendered strings and onto structured
result carriers so parser lookup can preserve semantic identity until spelling
is explicitly needed for bridge, AST text, debug, or diagnostics.

## Goal

Leave parser visible-name lookup with a clear structured result path:

- type, value, and concept visible lookup can report structured identity
- rendered spellings are compatibility or diagnostic products, not primary
  semantic lookup currency
- using-alias and namespace-stack paths preserve context and key identity
  instead of re-entering lookup through canonical strings
- existing string-returning surfaces survive only as compatibility wrappers or
  explicitly named bridge helpers

## Core Rule

Prefer structured parser identity over rendered spelling on semantic lookup
paths. Do not prove progress with testcase-shaped shortcuts, expectation
downgrades, or a one-off alias/namespace spelling repair.

## Read First

- [ideas/open/87_parser_visible_name_resolution_structured_result.md](/workspaces/c4c/ideas/open/87_parser_visible_name_resolution_structured_result.md)
- [ideas/closed/82_parser_namespace_textid_context_tree.md](/workspaces/c4c/ideas/closed/82_parser_namespace_textid_context_tree.md)
- [ideas/closed/83_parser_scope_textid_binding_lookup.md](/workspaces/c4c/ideas/closed/83_parser_scope_textid_binding_lookup.md)
- [ideas/closed/84_parser_qualified_name_structured_lookup.md](/workspaces/c4c/ideas/closed/84_parser_qualified_name_structured_lookup.md)
- [ideas/closed/86_parser_alias_template_structured_identity.md](/workspaces/c4c/ideas/closed/86_parser_alias_template_structured_identity.md)

## Current Targets

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- focused parser tests for namespace using, alias/template lookup, qualified
  member types, and concept/value/type visible-name resolution

## Non-Goals

- HIR/LIR link-name table redesign
- repo-wide string-to-`TextId` migration
- reopening completed lexical-scope, qualified-name, or alias-template
  identity work except where current callers consume visible-name results
- merging namespace traversal, lexical-scope lookup, and template-owned lookup
  into one combined subsystem
- expectation downgrades as the main proof mechanism
- testcase-shaped shortcuts for one alias, namespace, concept, value, or type
  spelling

## Working Model

- parser lookup tables may already contain structured keys or context; this
  runbook owns the result-passing layer that reports what visible lookup found
- semantic consumers should receive a structured result and ask for spelling
  only at an explicit boundary
- compatibility wrappers may continue returning `std::string` while the
  internal structured path is introduced and adopted
- bridge helpers such as `bridge_name_in_context(...)` and
  `qualified_name_text(...)` should become visibly bridge/debug/diagnostic
  tools, not semantic hot-path APIs

## Execution Rules

- classify call sites before editing the resolution API
- keep type/value/concept retargeting in separate packets unless a helper
  change requires a narrow shared carrier
- preserve behavior while changing representation; parser semantic fixes found
  during this work should become separate ideas unless required for this
  result-carrier route
- keep compatibility wrappers small and clearly layered over structured
  resolution
- use `build -> focused parser proof -> broader validation only when shared
  infrastructure or HIR-facing behavior changes`
- if execution reveals a distinct parser or frontend initiative, record it as
  a separate open idea instead of expanding this plan

## Step 1: Inventory Visible-Name Resolution Consumers

Goal: classify every visible-name lookup consumer by the kind of result it
actually needs.

Primary targets:

- `resolve_visible_type_name(...)`
- `resolve_visible_value_name(...)`
- `resolve_visible_concept_name(...)`
- `lookup_using_value_alias(...)`
- namespace-stack lookup helpers that publish canonical strings
- `bridge_name_in_context(...)` and `qualified_name_text(...)`

Actions:

- inspect declarations, definitions, and call sites for the visible-name
  helpers
- classify each call site as semantic identity, bridge spelling, diagnostic
  text, AST spelling, or fallback recovery
- identify the smallest shared structured carrier shape that can serve the
  semantic call sites without forcing unrelated parser cleanup
- choose the first implementation packet, preferably type lookup if it is the
  clearest semantic path

Completion check:

- `todo.md` records the consumer classification, the proposed carrier fields,
  and the first code-changing packet with proof scope

## Step 2: Introduce Structured Visible Resolution Carrier

Goal: add a structured visible-name result path while keeping existing string
surfaces available as compatibility wrappers.

Primary targets:

- parser-visible resolution declarations in `parser.hpp` or parser-private
  support headers
- parser state or qualified-name helpers needed to carry `TextId`, namespace
  context, owner path, or `QualifiedNameKey`
- existing string-returning helper entry points

Actions:

- define a narrow result carrier for visible type/value/concept resolution
- include semantic identity fields needed by real call sites found in Step 1
- add spelling accessors or compatibility wrappers that derive bridge text from
  the structured result
- preserve current public behavior while making the structured path available

Completion check:

- code builds with the structured carrier present
- existing string-returning callers still work through explicit compatibility
  paths

## Step 3: Retarget Visible Type Lookup

Goal: make type visible-name lookup consume and propagate structured identity
instead of canonicalized spelling where semantic identity is required.

Primary targets:

- namespace-qualified type lookup paths
- alias-template and template-owned member type lookup paths
- declarator and struct/type parsing consumers that currently round-trip
  through rendered names

Actions:

- retarget semantic type consumers to the structured result
- keep bridge/AST spelling callers on explicit derived-spelling paths
- add or update focused parser tests for namespace-qualified types,
  alias-template follow-through, and template-owned member types

Completion check:

- type lookup semantic paths no longer need to recover identity from rendered
  strings for the covered cases
- `c4c_frontend`, `c4cll`, and focused parser tests pass for the slice

## Step 4: Retarget Visible Value And Concept Lookup

Goal: apply the structured result path to value and concept visible-name
resolution without widening into unrelated parser cleanup.

Primary targets:

- value lookup through namespaces and using-visible paths
- concept lookup through namespaces and using-visible paths
- constructor-init visible-head behavior only where it consumes visible-name
  results directly

Actions:

- retarget semantic value consumers to structured visible-name results
- retarget semantic concept consumers to structured visible-name results
- preserve diagnostic and AST spelling behavior through explicit spelling
  derivation
- add focused parser proof for qualified visible-value calls and concept lookup

Completion check:

- value and concept semantic consumers use structured results for the covered
  paths
- focused parser tests prove no behavior regression in the retargeted cases

## Step 5: Narrow Using-Alias And Namespace-Stack Publishing

Goal: keep structured identity primary through using-alias and namespace-stack
lookup, rendering canonical spelling only at explicit boundaries.

Primary targets:

- `lookup_using_value_alias(...)`
- using declarations and namespace import flows
- namespace-stack lookup helpers that currently publish canonical strings as
  semantic results
- bridge helpers still called from semantic paths

Actions:

- make using-alias lookup publish structured target results
- preserve imported context and base `TextId` without re-entering lookup via
  `"A::B::name"` strings
- retarget namespace-stack semantic lookups to preserve context or qualified
  key identity
- leave bridge helpers narrow and visibly bridge-only

Completion check:

- using-alias and namespace-stack semantic paths avoid string re-entry for the
  covered cases
- bridge helper call sites are classified as bridge/debug/diagnostic/fallback
  rather than hidden semantic lookup

## Step 6: Prove Coverage And Decide Closure

Goal: validate the structured visible-name result route and decide whether the
source idea is complete or needs a follow-on initiative.

Primary targets:

- focused parser regressions for namespace-qualified type and value lookup
- using declarations and namespace imports
- alias-template and template-owned member type follow-through
- qualified visible-value calls and constructor-init visible-head consumers
- concept lookup through namespace and using-visible paths

Actions:

- run `cmake --build build -j --target c4c_frontend c4cll`
- run `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- escalate validation if shared qualified-name infrastructure or HIR-facing
  symbol/link-name behavior changed
- classify leftovers as either remaining idea-87 scope or a separate open idea

Completion check:

- covered visible-name lookup paths carry structured semantic identity through
  resolution and render spelling only at explicit boundaries
- proof results are recorded and any residual work is honestly split
