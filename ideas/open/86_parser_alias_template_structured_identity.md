# Parser Alias Template Structured Identity

Status: Proposed
Last Updated: 2026-04-24

## Goal

Move parser alias-template and related template-owned lookup off
spelling-shaped keys and ad-hoc rendered-name recovery onto structured
identity built from `TextId`, `QualifiedNameKey`, and explicit owner-path
state.

## Why This Idea Exists

Ideas 82 through 84 already moved namespace traversal, lexical scope lookup,
and qualified parser binding lookup toward structured identity. That work
reduced the old `"A::B"` hot path substantially, but one parser family still
leans heavily on rendered spelling and string-shaped recovery:

- alias template registration stores metadata in
  `ParserTemplateState::alias_template_info` keyed by `std::string`
- template-owned struct and instantiation tables still use several
  spelling-shaped maps and sets
- active parser context still mirrors some semantic names as both
  `std::string` and `TextId`
- alias template application still falls back to rendered-name lookup,
  string substitution, and alias-name-derived heuristics in places where the
  parser already has richer structured identity available

That leaves a noticeable holdout after idea 83:

- the parser can often parse and intern semantic identity correctly
- but alias/template follow-through still re-enters the system through
  rendered names
- later fixes in this area therefore risk growing more special cases instead
  of strengthening one authoritative lookup path

## Main Objective

Make alias-template and adjacent template-owned parser identity structured
first, so alias lookup, alias application, and template-owned member/type
follow-through stop depending on canonical rendered names as the primary key.

## Execution Note

- The active Step 5 parser packet completed the bridge narrowing in
  `parser_types_base.cpp`: structured owner/member lookup now runs before the
  `_t` / trait-style compatibility fallbacks, and the legacy bridge path is
  explicitly fallback-only.
- The prior runbook is exhausted. Any remaining parser identity work should be
  reactivated as a fresh plan only if it has a distinct execution boundary.

The intended direction is:

- keep lexical scope lookup separate from namespace traversal, as established
  by ideas 82 and 83
- represent alias-template identity with `TextId` plus owner-path-aware
  structured keys instead of plain `std::string`
- reduce parser active-context duplication where a semantic name is tracked
  as both mutable text and `TextId`
- keep rendered spellings as diagnostics, bridge output, or temporary
  compatibility only

## Primary Scope

- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- nearby parser tests that cover alias templates, `using` aliases, and
  template-owned member type resolution

## Concrete Problems To Remove

### A. Alias Template Metadata Keyed By Rendered Names

`ParserTemplateState::alias_template_info` is still keyed by `std::string`,
and registration currently flows through `set_last_using_alias_name(...)`
plus spelling-shaped lookup before template metadata is recorded.

Expected direction:

- store alias-template metadata under structured parser identity
- keep alias registration and alias lookup on the same semantic key family
- avoid re-resolving alias templates by rendered strings when `TextId` and
  owner path are already known

### B. Template-Owned Identity Tables Still Mixed Between Semantic And Spelling Forms

Several parser template and definition tables still use spelling-shaped keys,
including template struct registries, instantiation keys, and struct-tag
definition maps.

Expected direction:

- classify which tables truly need rendered names and which really represent
  semantic parser identity
- migrate the semantic tables toward `TextId`, `QualifiedNameKey`,
  `NamePathId`, or another explicit structured key
- keep diagnostics/debug rendering as a derived view rather than the storage
  authority

### C. Active Context Name State Still Mirrors `std::string` And `TextId`

`last_using_alias_name`, `last_resolved_typedef`, and `current_struct_tag`
each keep both a mutable string and a `TextId` copy.

Expected direction:

- reduce the parser's reliance on duplicated mutable text state when a stable
  semantic id is already present
- keep fallback spelling only where rollback, diagnostics, or legacy call
  sites still require it
- make tentative snapshot and restoration paths depend on one clearer
  identity model

### D. Alias Application Still Depends On String Recovery And Name-Derived Heuristics

Alias-template application in `parser_types_base.cpp` still performs several
rendered-name recovery steps and alias-name-derived special cases while
resolving aliased member types and trait-style aliases.

Expected direction:

- split semantic alias application from bridge-only spelling recovery
- prefer owner-path-aware structured identity over `rfind("::")`,
  `_t`-derived recovery, or similar alias-name heuristics
- keep temporary compatibility heuristics narrow and explicitly marked while
  the structured path takes over

## Constraints

- preserve parser behavior for existing alias-template and `using` alias
  coverage while changing the lookup substrate
- do not collapse namespace traversal, lexical scope, and template-owned
  lookup into one giant redesign
- do not turn this into a repo-wide string-to-`TextId` migration
- do not accept testcase-shaped shortcuts that only patch one alias family
  without strengthening the general lookup path

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Focused proof should include parser/frontend cases that exercise:

- `using Alias = ...` at top level and in local scope
- alias templates with type and non-type template parameters
- namespace-qualified alias lookup
- alias-member follow-through such as `Owner<T>::type` and `_t`-style paths
- template-owned struct references that currently depend on rendered names

Escalate beyond the parser subset if the slice starts touching shared
qualified-name infrastructure in a way that broadens the blast radius.

## Non-Goals

- no re-opening the completed lexical-scope migration from idea 83
- no full replacement of every parser/template string in one runbook
- no backend, HIR, or repo-wide semantic identity redesign
- no expectation downgrades as the main proof mechanism

## Suggested Execution Decomposition

1. Inventory parser alias/template tables and active-context fields by true
   identity kind.
2. Introduce one structured key family for alias-template registration and
   lookup.
3. Retarget alias-template registration from `using` alias bookkeeping onto
   that structured key path.
4. Migrate one template-owned lookup family at a time away from spelling-only
   tables.
5. Narrow the remaining alias application heuristics so they are bridge-only
   rather than the primary semantic route.

## Follow-On Relationship

This idea intentionally follows the parser identity sequence:

- `ideas/closed/82_parser_namespace_textid_context_tree.md`
- `ideas/closed/83_parser_scope_textid_binding_lookup.md`
- `ideas/closed/84_parser_qualified_name_structured_lookup.md`

and picks up a remaining holdout that those ideas did not finish:

- alias-template and template-owned structured identity

Idea 85 stays separate because constructor-init ambiguity decomposition is a
probe-boundary problem, not a parser identity/storage problem.
