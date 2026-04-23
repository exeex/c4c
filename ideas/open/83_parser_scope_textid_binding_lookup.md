# Parser Scope TextId Binding Lookup

Status: Open
Last Updated: 2026-04-23

## Goal

Add a parser lexical-scope binding system that uses `TextId`-native lookup as
the primary semantic path, while keeping namespace traversal as a separate
system and using `src/shared/local_name_table.hpp` as the main scope-local
lookup substrate.

## Why This Idea Exists

Idea 82 improves namespace ownership and qualified namespace traversal, but the
parser's visible-name lookup still mixes several responsibilities:

- lexical visibility is not modeled as its own scope-local binding system
- visible type/value/concept lookup still relies heavily on string-shaped
  bridge paths
- several parser binding tables still use `std::string` where semantic
  identity is already available as `TextId`
- namespace traversal and local visibility are not yet cleanly separated

That keeps `TextId` available but not fully on the hot path for parser lookup.
It also leaves many queries depending on composed spelling such as `"A::B"` or
other rendered-name bridges when the parser should instead query by structured
identity.

## Main Objective

Make parser-visible lookup `TextId`-first by introducing a dedicated lexical
scope lookup layer and migrating suitable binding tables away from
`std::string` keys toward `TextId`, `TextId` sequences, or interned path ids.

The intended direction is:

- keep namespace ownership and namespace-qualified traversal in the namespace
  tree introduced by idea 82
- add a separate parser scope-local binding model for lexical visibility
- use `src/shared/local_name_table.hpp` as the main scope-local lookup and
  binding strategy
- treat single-segment semantic names as `TextId` keys by default
- treat multi-segment names as `TextId` sequences or interned path ids rather
  than composed strings
- demote rendered strings to debug, diagnostics, and compatibility-only use

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser helper files that currently route visible type/value/concept lookup
- parser binding tables that still use `std::string` where `TextId` is already
  the real semantic identity
- `src/shared/local_name_table.hpp`
- `src/shared/qualified_name_table.hpp`
- `src/shared/text_id_table.hpp`

## Candidate Directions

1. Add a dedicated parser scope state with explicit push/pop behavior for
   lexical visibility, separate from namespace push/pop state.
2. Use `LocalNameTable` as the main scope-local binding substrate for parser
   type/value/concept lookup instead of growing more ad-hoc string-keyed local
   maps.
3. Audit parser binding tables and replace `std::string` keys with `TextId`
   wherever the table really represents single-segment semantic identity.
4. For multi-segment names, use `TextId` sequence keys or interned path ids
   instead of composing new `"A::B"` strings for semantic lookup.
5. Introduce a unified parser lookup facade so new `TextId`-first scope lookup
   can run in parallel with legacy bridge-based lookup during migration.
6. Keep `TextTable` as spelling storage and interning infrastructure, but do
   not treat rendered strings or raw hash values as semantic identity.

## Constraints

- preserve parser behavior while changing lookup structure
- keep namespace traversal and lexical scope lookup as separate systems with a
  clear boundary
- prefer `TextId` keys for single names and `TextId`-native sequence/path keys
  for multi-segment names
- use `src/shared/local_name_table.hpp` as the preferred strategy for new
  scope-local parser lookup
- treat string rendering as a bridge layer, not the primary semantic lookup
  path
- audit before migrating: not every existing `std::string` table should be
  replaced in the first packet
- do not widen this slice into sema, HIR, or backend identity redesign
- do not use raw hash values alone as semantic keys

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests that exercise:
  - local scope shadowing
  - block and function lexical visibility
  - namespace-qualified type/value lookup continuing to work through the
    separate namespace system
  - `using namespace` and `using` alias behavior through the unified lookup
    facade
  - template parameter and record-owner cases that depend on visible lookup
- broader `ctest` only if the slice crosses beyond parser lookup plumbing

## Non-Goals

- no full C++ name-lookup conformance pass in one idea
- no one-shot removal of every legacy string bridge
- no forced repo-wide `std::string` to `TextId` migration
- no collapse of namespace traversal and lexical visibility into one data
  structure
- no reliance on composed strings or raw hash values as the long-term semantic
  identity path
- no sema, HIR, or backend naming redesign

## Suggested Execution Decomposition

1. Inventory parser binding tables and classify which ones should stay string
   keyed, which should become `TextId` keyed, and which should move to
   `LocalNameTable` scope-local storage.
2. Add parser lexical scope state plus `LocalNameTable`-based local bindings
   for the simplest single-segment type/value/concept cases.
3. Introduce a unified `TextId`-first lookup facade that queries new
   scope-local bindings first and then falls back to the legacy bridge path.
4. Move unqualified visible lookup onto the new scope-local path while keeping
   namespace-qualified lookup on the separate namespace tree.
5. Replace remaining suitable single-name `std::string` binding tables with
   `TextId`-based storage and isolate any multi-segment semantic queries to
   `TextId` sequence or path-id keys.
6. Reduce legacy composed-string lookup to compatibility, debug, and
   diagnostic helpers once the new path proves equivalent on the focused test
   set.
