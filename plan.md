# Parser Scope TextId Binding Lookup

Status: Active
Source Idea: ideas/open/83_parser_scope_textid_binding_lookup.md

## Purpose
Move parser lexical visible-name lookup onto a dedicated `TextId`-first
scope-local path while keeping namespace traversal separate.

## Goal
Make parser-visible lexical lookup use `LocalNameTable` and `TextId`-native
keys as the primary semantic path.

## Core Rule
Do not collapse lexical scope lookup and namespace-qualified traversal into
one mechanism.

## Read First
- `ideas/open/83_parser_scope_textid_binding_lookup.md`
- `src/shared/local_name_table.hpp`
- `src/shared/qualified_name_table.hpp`
- `src/shared/text_id_table.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`

## Current Targets
- parser-visible type/value/concept lookup that is lexical and unqualified
- parser binding tables that still use `std::string` for single-segment
  semantic names
- scope-local parser helpers that should push and pop lexical bindings
- unified lookup helpers that should consult the new local scope path before
  bridge fallbacks

## Non-Goals
- no re-merging lexical scope lookup with namespace traversal
- no reopening the qualified or owner-scoped lookup slice completed under
  idea 84
- no giant repo-wide string-to-`TextId` migration
- no backend, HIR, or sema identity redesign
- no testcase-shaped shortcuts or expectation downgrades

## Working Model
- keep namespace ownership and namespace-qualified traversal on the separate
  namespace context tree
- add explicit parser lexical scope state with push/pop behavior for visible
  local bindings
- use `LocalNameTable` as the preferred substrate for lexical unqualified
  parser lookup
- treat single-segment semantic names as `TextId` keys by default
- keep rendered strings only for compatibility, diagnostics, and untouched
  bridge paths

## Execution Rules
- preserve parser behavior while changing lookup structure
- start with an inventory of the remaining parser binding tables before
  widening migration work
- keep qualified or owner-scoped tables on their structured path rather than
  pulling them back into this idea
- prefer build proof plus focused parser/frontend tests after each code step
- widen validation only if the blast radius extends beyond parser lookup
  plumbing

## Steps
1. Inventory parser binding tables for lexical lookup migration.
   - Classify remaining parser binding tables into `TextId`-native,
     direct-`TextId` migration, `LocalNameTable` scope-local, and
     structured-qualified holdout buckets.
   - Record the first concrete lexical migration packet in `todo.md`.
   - Completion check: the lexical lookup surface is partitioned and the next
     narrow packet is explicit.

2. Introduce parser lexical scope state for the simplest local bindings.
   - Add parser scope push/pop state that can own visible local bindings
     separately from namespace context state.
   - Start with the simplest unqualified type/value/concept cases that map
     cleanly to `LocalNameTable<TextId, ...>`.
   - Completion check: parser state can register and query at least one local
     binding family through scope-local `TextId` storage.

3. Add a unified `TextId`-first visible lookup facade.
   - Introduce parser-facing helpers that consult scope-local bindings first
     and then fall back to legacy bridge lookup when needed.
   - Keep namespace-qualified lookup routed through the namespace tree rather
     than the lexical-scope path.
   - Completion check: touched visible lookup call sites go through a
     structured-first facade.

4. Move unqualified visible lookup onto the new scope-local path.
   - Retarget the touched parser type/value/concept lookup paths so unqualified
     lexical visibility resolves through the new scope-local storage.
   - Preserve separate handling for namespace-qualified lookup and alias/import
     traversal.
   - Completion check: the touched unqualified lookup paths are lexical-scope
     first instead of bridge-string first.

5. Replace remaining suitable single-name string tables and isolate holdouts.
   - Migrate remaining single-segment semantic tables such as concepts,
     enum constants, or constant integer bindings to `TextId`-based storage
     where they are part of lexical visible lookup.
   - Leave multi-segment or owner-qualified queries on structured sequence or
     path-id keys rather than reintroducing flat strings.
   - Completion check: touched single-name parser bindings are `TextId`-first
     and the remaining holdouts are explicitly compatibility-only or
     structured-qualified.

6. Reduce legacy lexical bridge lookup to compatibility-only support.
   - Demote composed-string lexical lookup on touched paths to fallback,
     diagnostics, or compatibility support.
   - Keep rendered text available only where untouched bridge code still needs
     spelling.
   - Completion check: lexical lookup on the touched parser paths is
     structured-first and composed strings are no longer authoritative.

7. Re-run focused parser proof and widen only if the blast radius grows.
   - Build with `cmake --build --preset default`.
   - Run focused proof with
     `ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'`.
   - Completion check: focused parser/frontend proof passes for the updated
     lexical lookup path.
