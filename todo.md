Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Visible-Name Resolution Consumers
# Current Packet

## Just Finished

Step 1 inventory completed for visible-name resolution consumers.

Declarations inspected:

- `parser.hpp`: string-returning declarations for `resolve_visible_type_name`,
  `resolve_visible_value_name`, `resolve_visible_concept_name`,
  `lookup_using_value_alias`, `bridge_name_in_context`,
  `qualified_name_text`, `canonical_name_in_context`,
  `compatibility_namespace_name_in_context`, and `lookup_*_in_context`.
- `parser_state.hpp`: namespace carrier state, including
  `ParserNamespaceContext::{id,parent_id,is_anonymous,text_id,display_name,canonical_name}`,
  `ParserNamespaceState::namespace_stack`, namespace child/import maps, and
  `UsingValueAlias::{target_key,compatibility_name}`.

Definitions and helper classification:

- `resolve_visible_type_name(...)`: semantic identity hot path that currently
  returns rendered typedef spelling; it probes local typedef identity, walks
  namespace stack, follows using value alias as a compatibility bridge, calls
  `lookup_type_in_context`, and falls back through `probe_visible_typedef_name`.
- `resolve_visible_value_name(...)`: semantic identity hot path for variable
  and function lookup; it probes local value identity, walks namespace stack,
  uses `lookup_using_value_alias`, and calls `lookup_value_in_context`.
- `resolve_visible_concept_name(...)`: semantic identity hot path for concept
  lookup; it preserves unqualified local concept text ids, then walks namespace
  stack through `lookup_concept_in_context`.
- `lookup_using_value_alias(...)`: semantic alias result with a string bridge;
  it already stores `QualifiedNameKey target_key` but publishes a rendered
  string or `compatibility_name`.
- Namespace-stack helpers that publish canonical/rendered strings:
  `resolve_visible_name_from_namespace_stack` returns `std::string`;
  `lookup_value_in_context`, `lookup_type_in_context`, and
  `lookup_concept_in_context` publish rendered strings from `QualifiedNameKey`;
  `render_lookup_name_in_context`, `bridge_name_in_context`,
  `compatibility_namespace_name_in_context`, and `canonical_name_in_context`
  render bridge/debug compatibility spelling from context plus `TextId`.
- `qualified_name_text(...)`: explicit AST/source spelling builder for parsed
  `QualifiedNameRef`; fallback text for unresolved qualified values and AST
  name storage, not primary semantic identity.

Call-site classification:

- Semantic identity:
  `parser_core.cpp` `find_visible_typedef_type`,
  `find_visible_var_type`, `find_alias_template_info_in_context`,
  `is_concept_name`, `resolve_qualified_value_name`,
  `resolve_qualified_type_name`, `lookup_value_in_context`,
  `lookup_type_in_context`, `lookup_concept_in_context`;
  `parser_declarations.cpp` `same_resolved_type_name` and using-declaration
  import registration; `types_helpers.hpp` type resolution probes;
  `parser_types_template.cpp` alias-template/template-primary probes;
  `parser_types_declarator.cpp` dependent typedef resolution;
  `parser_types_base.cpp` expression-vs-type head disambiguation, typedef tag
  resolution, and template owner lookup; `parser_expressions.cpp` direct
  typedef cast disambiguation.
- Bridge spelling:
  `bridge_name_in_context`, `render_lookup_name_in_context`,
  `compatibility_namespace_name_in_context`, and callers that register legacy
  string keys for typedefs, template primaries, using imports, enum refs, and
  struct tags.
- Diagnostic/debug or compatibility text:
  `canonical_name_in_context`, `refresh_current_namespace_bridge`, and
  namespace `canonical_name` publication; no current inspected call site needs
  these as a semantic primary result.
- AST spelling:
  `qualified_name_text` in `parser_types_declarator.cpp` output name capture,
  `parser_types_struct.cpp` qualified tag spelling, and qualified expression
  fallback node names in `parser_expressions.cpp`.
- Fallback recovery:
  unresolved qualified value fallbacks in `parser_expressions.cpp`,
  `types_helpers.hpp` unresolved qualified fallback probes, `resolve_visible_type_name`
  fallback through `probe_visible_typedef_name`, and bridge fallback when a
  structured key cannot render.

Smallest proposed structured carrier fields:

- `bool found`
- `enum class VisibleNameKind { Type, Value, Concept }`
- `QualifiedNameKey key`
- `TextId base_text_id`
- `int context_id`
- `enum class VisibleNameSource { Local, Namespace, UsingAlias, ImportedNamespace, AnonymousNamespace, Fallback }`
- `std::string compatibility_spelling` only for wrappers, AST text,
  diagnostics, and legacy string-keyed tables while callers are retargeted.

## Suggested Next

First code-changing packet: introduce the visible-name result carrier and a
type-only structured resolver, then keep `resolve_visible_type_name(...)` as a
compatibility wrapper that renders from the structured result. Limit adoption
to type lookup internals with the clearest semantic dependency:
`find_visible_typedef_type`, `resolve_qualified_type_name`,
`lookup_type_in_context`, and the direct type probes in `types_helpers.hpp`.
Proof scope should be build plus focused parser/frontend tests covering
namespace-qualified typedefs, using-imported typedefs, alias-template
follow-through, and qualified member type parsing.

## Watchouts

- Keep source idea 87 focused on visible-name result representation, not a
  parser-wide semantic cleanup.
- Do not repair one namespace, alias, value, type, or concept spelling with a
  testcase-shaped shortcut.
- Preserve string-returning helpers as compatibility wrappers until their
  callers have been intentionally retargeted.
- Do not mix this semantic parser identity route with parser/HIR header
  hierarchy work from ideas 92 or 93.
- `lookup_using_value_alias` is named value-only, but `resolve_visible_type_name`
  currently consults it before type lookup. The next packet should preserve
  behavior while making the alias target key, not its rendered spelling, the
  semantic handoff.
- `qualified_name_text` has legitimate AST/fallback uses; do not remove it
  from node-name construction just because semantic lookup gets a structured
  result.

## Proof

No build or test run; delegated proof explicitly said this is an inventory-only
packet. Proof scope was source inspection with `rg` over `src/frontend/parser`
and targeted reads of `parser.hpp`, `parser_state.hpp`, `parser_core.cpp`,
`types_helpers.hpp`, `parser_declarations.cpp`, `parser_expressions.cpp`,
`parser_types_base.cpp`, `parser_types_declarator.cpp`,
`parser_types_struct.cpp`, and `parser_types_template.cpp`. No `test_after.log`
was created because there was no delegated build/test command.
