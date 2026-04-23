# Parser Qualified Name Structured Lookup

Status: Active
Source Idea: ideas/open/84_parser_qualified_name_structured_lookup.md
Activated from: ideas/open/83_parser_scope_textid_binding_lookup.md

## Purpose
Move parser qualified-name and namespace-side semantic lookup off rendered-string bridges onto structured identity built from `TextId` segments and interned path ids, while keeping lexical scope lookup and namespace traversal separate.

## Goal
Make parser qualified-name semantic lookup use structured identity as the primary path.

## Core Rule
Do not collapse lexical scope lookup and namespace traversal into one mechanism.

## Read First
- `ideas/open/84_parser_qualified_name_structured_lookup.md`
- `src/shared/qualified_name_table.hpp`
- `src/shared/text_id_table.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`

## Current Targets
- parser helper code that currently routes semantic lookup through rendered qualified-name strings
- `ParserBindingState::known_fn_names`
- `ParserBindingState::struct_typedefs`
- `qualified_name_text(...)`
- `compatibility_namespace_name_in_context(...)`

## Non-Goals
- no re-merging lexical scope lookup with namespace traversal
- no giant repo-wide string-to-`TextId` migration
- no requirement to delete every compatibility rendering helper in one pass
- no backend or HIR identity redesign outside what parser lookup directly needs
- no testcase-shaped shortcuts that only special-case a few known qualified names

## Working Model
- keep lexical-scope binding on the separate local-scope path from idea 83
- keep namespace ownership and traversal on the namespace context tree from idea 82
- represent qualified semantic identity with `TextId` sequences, `NamePathId`, or explicit structured keys instead of canonical strings
- demote rendered-name helpers to diagnostics, bridge code, and compatibility-only paths

## Execution Rules
- preserve parser behavior while changing lookup structure
- keep namespace-qualified lookup on the structured namespace path, not the lexical scope path
- start with an inventory of the remaining qualified parser binding tables
- keep the slice narrow enough to stay inside parser lookup plumbing
- validate each code step with build proof and focused parser/frontend tests
- do not widen the slice into lexical unqualified lookup work from idea 83

## Steps
1. Inventory remaining qualified parser binding tables.
   - Classify the still-string-keyed qualified or owner-scoped tables and helper paths.
   - Identify the first safe structured-key migration targets and the tables that should stay bridge-only for now.
   - Completion check: the qualified lookup surface is partitioned into structured-key targets and compatibility-only holdouts.

2. Introduce structured qualified-name keys for parser-owned lookup.
   - Add or normalize explicit structured key types for owner-scoped parser lookup.
   - Replace the first narrow qualified binding family with structured storage.
   - Completion check: at least one qualified binding family no longer relies on rendered strings as its primary semantic key.

3. Retarget `using` / alias / import lookup paths to structured identity.
   - Move qualified import and alias registration off rendered-name reconstruction.
   - Keep lexical scope lookup and namespace visibility separate while changing the parser-owned lookup key.
   - Completion check: structured identity reaches the parser paths that previously flattened qualified names back to strings.

4. Reduce compatibility rendering to bridge-only support on touched paths.
   - Demote `qualified_name_text(...)` and `compatibility_namespace_name_in_context(...)` away from primary semantic lookup.
   - Keep rendered text only where diagnostics or legacy compatibility still require spelling.
   - Completion check: the remaining qualified lookup paths are structured-first and the rendered bridge is no longer authoritative.

5. Re-run focused parser proof and widen only if the blast radius grows.
   - Build the parser frontend and run the focused parser test subset that covers qualified name lookup.
   - Escalate to broader checks only if the touched surface expands beyond parser lookup plumbing.
   - Completion check: focused proof passes on the structured qualified-lookup path.
