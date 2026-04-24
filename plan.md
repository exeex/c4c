# Parser Alias Template Structured Identity

Status: Active
Source Idea: ideas/open/86_parser_alias_template_structured_identity.md
Last Updated: 2026-04-24

## Purpose
Move parser alias-template and adjacent template-owned lookup off rendered-name recovery and onto structured identity built from `TextId`, `QualifiedNameKey`, and explicit owner-path state.

## Goal
Keep alias lookup, alias application, and template-owned member/type follow-through on one authoritative structured path, with rendered spellings retained only for diagnostics and bridge behavior.

## Core Rule
Do not turn this into a repo-wide string-to-`TextId` migration or a testcase-shaped shortcut. Strengthen the general lookup path, one template-owned family at a time.

## Read First
- `ideas/open/86_parser_alias_template_structured_identity.md`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`

## Current Scope
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_declarator.cpp`
- `src/frontend/parser/parser_types_template.cpp`
- parser tests that cover alias templates, `using` aliases, and template-owned member type resolution

## Non-Goals
- Do not reopen the lexical-scope migration from idea 83.
- Do not collapse namespace traversal, lexical scope, and template-owned lookup into one redesign.
- Do not accept one-off alias-family special cases as the main fix.
- Do not broaden beyond parser/frontend scope unless shared qualified-name infrastructure forces it.

## Working Model
- Treat `TextId`, `QualifiedNameKey`, and owner-path state as the semantic key family.
- Treat rendered names as derived output, temporary compatibility, or diagnostics.
- Migrate one lookup table or context field family per step.
- Keep behavior stable while moving the storage substrate.

## Execution Rules
- Inspect and inventory before changing lookup storage.
- Prefer structured keys over spelling-shaped maps and heuristics.
- Keep alias-template registration and lookup on the same semantic key path.
- If a step broadens blast radius, pause and tighten the plan before continuing.
- Validate each code-bearing step with build proof, then focused parser tests.

## Steps
### 1. Inventory parser alias/template identity surfaces
Goal: separate true semantic identity from spelling-shaped storage.
Primary Target: `parser_state.hpp`, `parser.hpp`, `parser_types_base.cpp`, `parser_types_template.cpp`
Actions:
- list alias-template metadata, template-owned registries, and active-context fields
- classify each table or field by semantic key, spelling key, or bridge-only output
- identify the minimum key family needed for alias-template registration and lookup
Completion Check:
- a concrete inventory exists for the touched parser families, with no code changes yet

### 2. Introduce one structured key family for alias-template registration and lookup
Goal: define the first authoritative structured route for alias-template identity.
Primary Target: parser template state and alias registration path
Actions:
- add or refit the parser-side key path to use `TextId` plus owner-path-aware structured identity
- keep compatibility shims narrow and explicit if older call sites still need spelling
- avoid adding new rendered-name recovery paths
Completion Check:
- alias-template registration and lookup can use the same structured key family end to end

### 3. Retarget alias-template registration away from spelling-shaped bookkeeping
Goal: stop recording alias-template metadata through rendered-name recovery.
Primary Target: alias registration flow in parser core/declaration handling
Actions:
- move registration off `std::string`-keyed alias metadata where the structured key is already available
- align `set_last_using_alias_name(...)` style bookkeeping with the new semantic key path
- preserve existing behavior for `using` alias coverage
Completion Check:
- alias-template metadata is no longer primarily keyed by rendered spelling

### 4. Migrate one template-owned lookup family at a time
Goal: replace the most semantic template-owned spelling tables with structured identity.
Primary Target: template-owned struct/instantiation lookup and related parser tables
Actions:
- pick one table family and convert it to the structured key path
- keep diagnostics and debug rendering derived from the structured storage
- leave unrelated spelling maps alone unless they are part of the same semantic path
Completion Check:
- at least one template-owned lookup family no longer depends on rendered-name storage

### 5. Narrow bridge-only heuristics and validate parser coverage
Goal: reduce alias application fallbacks to compatibility-only behavior.
Primary Target: alias application in `parser_types_base.cpp`
Actions:
- remove or narrow `rfind("::")`, `_t`-style recovery, and alias-name-derived heuristics where structured identity is available
- keep any remaining fallback logic explicitly marked as bridge-only
- run focused parser/frontend tests after the code changes
Completion Check:
- focused alias-template and `using` alias tests pass, and the remaining heuristics are clearly compatibility-only

## Validation
- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- broaden validation if a step touches shared qualified-name infrastructure
