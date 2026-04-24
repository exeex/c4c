# Parser Alias Template Structured Identity
Status: Active
Source Idea: ideas/open/86_parser_alias_template_structured_identity.md
Activated From: exhausted parser alias structured-identity runbook

## Purpose
Move parser alias-template and adjacent template-owned lookup off spelling-shaped recovery and onto structured identity.

## Goal
Make alias-template registration, lookup, and member follow-through prefer `TextId` / structured owner-path keys before rendered-name heuristics.

## Core Rule
Rendered spellings are diagnostics or fallback bridges only. They are not the primary storage or lookup authority when structured identity is already available.

## Read First
- `ideas/open/86_parser_alias_template_structured_identity.md`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_types_base.cpp`

## Current Targets
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- parser frontend tests covering alias templates, `using` aliases, and template-owned member type resolution

## Non-Goals
- Do not reopen the completed lexical-scope migration from idea 83.
- Do not turn this into a repo-wide string-to-`TextId` migration.
- Do not collapse namespace traversal, lexical scope, and template-owned lookup into one redesign.
- Do not accept testcase-shaped shortcuts that only patch one alias family.

## Working Model
- Treat structured identity as the normal path.
- Keep spelling-shaped recovery narrow, explicit, and marked as fallback.
- Preserve existing parser behavior while changing the lookup substrate.

## Execution Rules
- Make each step small enough to validate with parser/frontend tests.
- Prefer semantic repair over named-case special handling.
- If a change broadens beyond parser alias/template identity, stop and split the work into a separate idea.

## Steps

### Step 1
Inspect parser alias/template tables and active-context fields by true identity kind.

Primary targets:
- `parser_state.hpp`
- `parser.hpp`
- `parser_core.cpp`
- `parser_declarations.cpp`
- `parser_types_base.cpp`
- `parser_types_template.cpp`
- `parser_types_declarator.cpp`

Actions:
- classify each table or field as semantic identity, spelling bridge, or diagnostics-only
- identify the first structured key family that can replace a string-keyed alias/template path
- note any fallback-only spelling recovery that must remain temporarily

Completion check:
- the active lookup and state surfaces are categorized enough to support one concrete structured-key migration without widening scope

### Step 2
Introduce a structured key path for alias-template registration and lookup.

Primary targets:
- alias-template metadata and lookup code in the parser template state path
- registration flow that currently depends on `using` alias bookkeeping and rendered-name recovery

Actions:
- move alias-template metadata onto the chosen structured key family
- keep alias registration and alias lookup on the same semantic key path
- preserve compatibility behavior only as a fallback bridge

Completion check:
- alias-template lookup resolves through structured identity first, with fallback-only spelling recovery still working

### Step 3
Retarget template-owned member and type follow-through away from spelling recovery where structured identity already exists.

Primary targets:
- `parser_types_base.cpp`
- nearby template-owned lookup and member-resolution helpers

Actions:
- prefer owner-path-aware identity over `rfind("::")` and `_t`-style recovery when the semantic key is already known
- keep legacy heuristics explicitly marked as fallback-only
- validate the resulting behavior on parser/frontend coverage

Completion check:
- parser frontend tests pass on the focused alias-template and member-resolution coverage, and the remaining heuristics are clearly bridge-only

## Validation
- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

