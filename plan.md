# Parser Scope TextId Binding Lookup

Status: Active
Source Idea: ideas/open/83_parser_scope_textid_binding_lookup.md
Activated from: ideas/open/83_parser_scope_textid_binding_lookup.md

## Purpose
Move parser-visible lookup toward `TextId`-native lexical scope binding while keeping namespace traversal separate and preserving behavior during migration.

## Goal
Make parser lookup `TextId`-first for scope-local visibility, with `src/shared/local_name_table.hpp` as the main substrate and string rendering kept as compatibility-only support.

## Core Rule
Do not collapse namespace traversal and lexical scope lookup into one mechanism.

## Read First
- `ideas/open/83_parser_scope_textid_binding_lookup.md`
- `src/shared/local_name_table.hpp`
- `src/shared/qualified_name_table.hpp`
- `src/shared/text_id_table.hpp`

## Current Targets
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser helper files that still route visible type/value/concept lookup
- parser binding tables that still use `std::string` where `TextId` is the real semantic identity

## Non-Goals
- no full C++ name-lookup conformance pass in one idea
- no one-shot removal of every legacy string bridge
- no forced repo-wide `std::string` to `TextId` migration
- no sema, HIR, or backend naming redesign
- no collapse of namespace traversal and lexical visibility into one data structure

## Working Model
- keep namespace ownership and qualified traversal in the separate namespace tree from idea 82
- add parser lexical scope state for local bindings
- prefer `TextId` for single-segment semantic names
- use `TextId` sequences or interned path ids for multi-segment names
- treat rendered strings as debug, diagnostics, or compatibility bridges only

## Execution Rules
- preserve parser behavior while changing lookup structure
- keep scope-local lookup and namespace lookup separate
- start with inventory before migrating tables
- use `LocalNameTable` for new scope-local parser lookup where it fits
- validate each code step with build proof and focused parser/frontend tests
- do not widen the slice beyond parser lookup plumbing

## Steps
1. Inventory parser binding tables.
   - Classify each table as string-keyed, `TextId`-keyed, or `LocalNameTable`-backed.
   - Completion check: the inventory identifies the first migration targets and the tables that must remain unchanged for now.

2. Add parser lexical scope state for local bindings.
   - Introduce explicit push/pop behavior for lexical visibility separate from namespace state.
   - Completion check: the parser can represent scope-local bindings without reusing namespace traversal state.

3. Introduce a unified `TextId`-first lookup facade.
   - Query new scope-local bindings first, then fall back to legacy bridge paths.
   - Completion check: unqualified visible lookup has one entry point that prefers `TextId`-native state.

4. Move unqualified visible lookup onto the new scope-local path.
   - Keep namespace-qualified lookup on the separate namespace tree.
   - Completion check: local visibility works through the new path while namespace-qualified lookup still works through the old namespace system.

5. Replace remaining suitable single-name string tables.
   - Switch single-segment semantic identity to `TextId`-based storage where it is safe.
   - Isolate multi-segment semantic queries to `TextId` sequences or path ids.
   - Completion check: remaining `std::string` usage is limited to compatibility, diagnostics, or tables that genuinely need spelling.

6. Reduce legacy composed-string lookup to compatibility-only support.
   - Keep bridge-based lookup only where migration still requires it.
   - Completion check: the focused parser/frontend test set passes with the new lookup structure and the legacy bridge is no longer the primary semantic path.
