# Parser Visible Name Resolution Structured Result

Status: Open
Created: 2026-04-24
Last Updated: 2026-04-24
Parent Ideas:
- [84_parser_qualified_name_structured_lookup.md](/workspaces/c4c/ideas/closed/84_parser_qualified_name_structured_lookup.md)
- [86_parser_alias_template_structured_identity.md](/workspaces/c4c/ideas/closed/86_parser_alias_template_structured_identity.md)

## Goal

Move parser visible-name resolution results off rendered strings and onto a
structured result carrier, so type/value/concept lookup can preserve
`TextId`, namespace context, and qualified-key identity until a true bridge or
diagnostic spelling is required.

## Why This Idea Exists

Ideas 82 through 84 moved namespace traversal, lexical-scope binding, and
qualified parser binding lookup toward structured identity. Idea 86 then
finished the next parser holdout by moving alias-template, template-struct,
and active-context identity cleanup onto structured lookup paths.

After those closures, the remaining parser identity pressure is no longer just
one table storing the wrong key. The wider problem is that parser visible-name
resolution still often returns a rendered spelling as the primary result:

- `resolve_visible_type_name(...)`
- `resolve_visible_value_name(...)`
- `resolve_visible_concept_name(...)`
- `lookup_using_value_alias(...)`
- namespace-stack lookup helpers that publish canonical strings
- bridge helpers such as `bridge_name_in_context(...)` and
  `qualified_name_text(...)`

That means the parser can parse and store structured identity correctly, but
then still hands later code a string-shaped answer. Callers that really need a
semantic identity must recover it from spelling, while callers that only need
diagnostics or legacy AST text cannot be distinguished from semantic lookup
consumers.

## Main Objective

Introduce and consume a structured visible-name resolution result for parser
lookup.

The intended direction is:

- visible-name lookup should preserve `TextId`, namespace context, owner path,
  and/or `QualifiedNameKey` as the semantic result
- rendered spellings should become bridge/debug/diagnostic output, not the
  main lookup currency
- using-alias and namespace-stack lookup should carry structured identity
  through the resolution path instead of canonicalizing to text first
- existing compatibility surfaces should remain available while their role is
  narrowed and explicit

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- focused parser tests for namespace using, alias/template lookup,
  qualified member types, and concept/value/type visible-name resolution

## Concrete Problems To Remove

### A. Visible Resolution Returns Only A Rendered Name

The parser has structured lookup keys available, but the public visible-name
helpers mostly return `std::string`. That makes semantic lookup and diagnostic
spelling look like the same operation.

Expected direction:

- introduce a result carrier for visible type/value/concept resolution
- include structured semantic identity in the carrier
- keep bridge spelling as an optional derived field or helper result
- let string-returning helpers become compatibility wrappers over the
  structured result

### B. Using-Alias Lookup Re-enters Through String Keys

Using-alias and namespace import flows still lean on `bridge_name_in_context`
and canonical rendered names when they already know the context and base
`TextId`.

Expected direction:

- make using-alias lookup publish a structured target result
- keep imported identity separate from fallback spelling
- avoid converting `context + TextId` into `"A::B::name"` just to enter another
  parser lookup table

### C. Namespace Stack Lookup Publishes Canonical Text Too Early

Namespace-stack lookup still often builds canonical strings as the result of
semantic lookup. That is useful for bridge consumers, but too early for
structured parser identity.

Expected direction:

- preserve namespace context/path identity through namespace-stack lookup
- render canonical text only when a caller explicitly asks for bridge spelling
- keep `namespace_state_.current_namespace` and related canonical-name fields
  as compatibility state rather than primary lookup authority

### D. Bridge Helpers Are Still Semantic Hot-Path Tools

Helpers such as `bridge_name_in_context(...)` and `qualified_name_text(...)`
are still called from semantic paths, which makes it hard to tell whether a
call site is doing real lookup or just spelling output.

Expected direction:

- classify call sites as semantic lookup, legacy bridge, diagnostic, AST
  spelling, or fallback recovery
- retarget semantic lookup sites to the structured result
- leave bridge helpers narrow and visibly bridge-only

## Suggested Execution Decomposition

1. Inventory visible-name resolution call sites by true consumer kind:
   semantic identity, bridge spelling, diagnostic text, or fallback recovery.
2. Introduce a structured visible-name result carrier and compatibility
   wrappers for existing string-returning helpers.
3. Retarget type lookup first, especially namespace and alias-template member
   type paths that currently round-trip through rendered names.
4. Retarget value and concept lookup paths without widening into unrelated
   parser cleanup.
5. Narrow using-alias and namespace-stack helpers so structured identity stays
   primary and spelling is derived only at bridge boundaries.
6. Add focused parser regressions and run the frontend parser proof.

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Focused proof should include parser/frontend cases that exercise:

- namespace-qualified type and value lookup
- `using` declarations and namespace imports
- alias-template and template-owned member type follow-through
- qualified visible-value calls and constructor-init visible-head behavior
- concept lookup through namespace and using-visible paths

Escalate beyond the parser subset if the slice touches shared qualified-name
infrastructure or HIR-facing symbol/link-name behavior.

## Non-Goals

- no HIR/LIR link-name table redesign
- no repo-wide string-to-`TextId` migration
- no reopening completed lexical-scope, qualified-name, or alias-template
  identity work except where their callers consume visible-name results
- no merging namespace traversal, lexical scope lookup, and template-owned
  lookup into one combined subsystem
- no expectation downgrades as the main proof mechanism
- no testcase-shaped shortcut for one alias, namespace, or concept spelling

## Follow-On Relationship

This idea intentionally follows the parser structured-identity sequence:

- `ideas/closed/82_parser_namespace_textid_context_tree.md`
- `ideas/closed/83_parser_scope_textid_binding_lookup.md`
- `ideas/closed/84_parser_qualified_name_structured_lookup.md`
- `ideas/closed/86_parser_alias_template_structured_identity.md`

Those ideas moved major parser lookup tables and template identity paths toward
structured keys. This idea owns the remaining result-passing layer: how
visible-name lookup reports what it found before any compatibility spelling is
needed.

Idea 85 remains separate because constructor-init visible-head handling is a
probe-boundary ambiguity, not a visible-name result representation problem.
