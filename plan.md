# Parser Template Struct Structured Identity
Status: Active
Source Idea: ideas/open/86_parser_alias_template_structured_identity.md
Supersedes: exhausted alias-template/member-follow-through runbook

## Purpose
Finish the parser alias/template identity holdout by retargeting the remaining
`template_struct_*` primary, specialization, and instantiation bridges away
from spelling-owned maps where structured identity is already available.

## Goal
Make template-struct registration, lookup, specialization selection, and
instantiation prefer `QualifiedNameKey` / `TextId` owner-aware identity before
legacy string maps.

## Core Rule
Rendered spellings are diagnostics, compatibility bridges, or fallback probes
only. They must not become the semantic source of truth when a structured
parser identity is available.

## Read First
- `ideas/open/86_parser_alias_template_structured_identity.md`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_types_template.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_declarations.cpp`

## Current Targets
- `ParserTemplateState::template_struct_defs_by_key`
- `ParserTemplateState::template_struct_specializations_by_key`
- `ParserTemplateState::template_struct_defs`
- `ParserTemplateState::template_struct_specializations`
- `ParserTemplateState::instantiated_template_struct_keys`
- `find_template_struct_primary(...)`
- `find_template_struct_specializations(...)`
- `register_template_struct_primary(...)`
- `register_template_struct_specialization(...)`
- `ensure_template_struct_instantiated_from_args(...)`

## Non-Goals
- Do not reopen the completed lexical-scope, namespace traversal, or qualified
  parser binding migrations from ideas 82 through 84.
- Do not turn this into a repo-wide string-to-`TextId` migration.
- Do not remove compatibility string maps before proving all existing parser
  call sites have structured-key coverage.
- Do not accept testcase-shaped shortcuts that only patch one named template
  struct or alias family.

## Working Model
- Structured maps are the semantic authority.
- String maps remain bridge-only until every parser caller can supply the
  structured key it already knows.
- Instantiation keys should stay stable for existing behavior, but lookup
  should avoid re-entering through rendered names when a primary template node
  or structured owner key is available.

## Execution Rules
- Keep each step narrow enough for parser/frontend validation.
- Prefer adding structured overloads or threading existing `QualifiedNameKey`
  values over reparsing rendered names.
- Mark legacy string lookups as fallback-only where they remain.
- If a change broadens into shared qualified-name infrastructure or HIR/backend
  identity, stop and split the work into a separate idea.

## Steps

### Step 1
Inventory the remaining `template_struct_*` string bridge callers.

Primary targets:
- `parser_state.hpp`
- `parser.hpp`
- `parser_types_template.cpp`
- `parser_types_base.cpp`
- `parser_declarations.cpp`

Actions:
- classify each `template_struct_defs` and `template_struct_specializations`
  use as semantic storage, fallback bridge, diagnostics/debug output, or
  compatibility lookup
- identify call sites that already have `TextId`, `QualifiedNameRef`, context
  id, owner node, or primary template node available before falling back to
  rendered strings
- identify the first migration path that can avoid changing parser behavior

Completion check:
- the next code-changing step has a concrete caller family and structured-key
  source, with string-map uses explicitly categorized as fallback or still
  semantic

### Step 2
Retarget template-struct primary registration and lookup to structured keys.

Primary targets:
- `register_template_struct_primary(...)`
- `find_template_struct_primary(...)`
- declaration and type parsing call sites that pass rendered names

Actions:
- thread existing context id, `TextId`, or `QualifiedNameRef` data into primary
  registration and lookup where it is already available
- keep `template_struct_defs_by_key` authoritative for structured primary
  storage
- keep `template_struct_defs` writes and probes as bridge fallback only
- preserve compatibility behavior for callers that genuinely have only a
  spelling

Completion check:
- primary template-struct lookup resolves through structured identity first for
  parser paths that already know the owner/name identity, and string lookup is
  documented as fallback-only

### Step 3
Retarget template-struct specialization lookup to the same structured key path.

Primary targets:
- `register_template_struct_specialization(...)`
- `find_template_struct_specializations(...)`
- specialization selection paths in `parser_types_base.cpp` and
  `parser_declarations.cpp`

Actions:
- register specializations under the same structured primary identity used for
  primary lookup
- prefer structured specialization lookup when a primary node, context id,
  `TextId`, or `QualifiedNameRef` is available
- keep `template_struct_specializations` as compatibility fallback only
- avoid duplicating specializations through both rendered and qualified strings
  as if both were semantic authorities

Completion check:
- specialization selection reaches the structured specialization table before
  string-map fallback in the focused parser/template paths

### Step 4
Narrow instantiation and mangled-name bridge behavior around structured
template identity.

Primary targets:
- `ensure_template_struct_instantiated_from_args(...)`
- `make_template_struct_instance_key(...)`
- `build_template_struct_mangled_name(...)`
- callers that instantiate from alias/member follow-through

Actions:
- keep emitted/mangled names stable unless a semantic bug requires a change
- prefer primary template node or structured primary identity for deciding
  whether an instantiation already exists
- leave `instantiated_template_struct_keys` string-shaped only if it is purely
  an emitted-artifact key rather than parser identity storage
- document any remaining string key as bridge/output, not lookup authority

Completion check:
- instantiation decisions no longer require a rendered-name lookup when the
  parser already has structured template identity or primary template node
  state

## Validation
- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Escalate beyond parser frontend tests if a step changes shared qualified-name
infrastructure or touches non-parser semantic identity.
