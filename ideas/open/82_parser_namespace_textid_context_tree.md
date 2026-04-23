# Parser Namespace TextId Context Tree

Status: Open
Last Updated: 2026-04-23

## Goal

Replace parser namespace lookup's canonical-string-driven path with a namespace
context tree that resolves qualified names segment-by-segment via `TextId`,
while keeping namespace push/pop as the active registration and visibility
mechanism.

## Why This Idea Exists

The parser already has a namespace context stack and context ids, but the
current lookup model still depends heavily on canonical string assembly:

- named namespace children are indexed through composed string keys
- qualified-name resolution still falls back to `"A::B"`-style canonical text
- visible type/value/concept lookup often rebuilds canonical names before
  consulting parser tables

That leaves namespace ownership, lexical visibility, and rendered names mixed
together. It also makes later parser `TextId` cleanup harder than it should
be, because the lookup path itself still depends on string composition.

## Main Objective

Make namespace ownership and qualified-name traversal operate on structured
context + `TextId` segments rather than canonical-name strings.

The intended direction is:

- keep namespace registration/lifetime modeled by `push_namespace_context()` /
  `pop_namespace_context()`
- make namespace storage a real parent/child context tree
- resolve `A::B::C` by walking `TextId` segments one step at a time
- demote canonical `"A::B::C"` strings to debug, diagnostics, and bridge-only
  helpers instead of primary semantic lookup keys

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- nearby parser helper files that participate in qualified-name and namespace
  lookup

## Candidate Directions

1. Change namespace child registration from string-composed global maps to
   parent-context child maps keyed by `TextId`.
2. Extend `ParserNamespaceContext` to carry segment identity directly
   (`TextId`, optional display/debug spelling) rather than relying on canonical
   string reconstruction as the primary identity.
3. Treat `QualifiedNameRef` as a segment list first:
   `qualifier_text_ids + base_text_id`; keep string spelling only as a bridge.
4. Rewrite `resolve_namespace_context()` / `resolve_namespace_name()` to walk
   the namespace tree segment-by-segment from the active stack or global root.
5. Keep `canonical_name_in_context()` and related string helpers only as
   rendering/debug bridges until later follow-on work removes more string
   fallback paths.

## Constraints

- preserve parser behavior while changing lookup structure
- keep namespace push/pop as the active context model; do not mix this slice
  with full lexical-scope redesign
- do not try to replace every parser string-keyed binding table in the same
  runbook
- do not absorb sema, HIR, or backend identity work here
- prefer explicit `TextId` segment traversal over `"A::B"` composed-string
  hashing

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests that exercise:
  - qualified namespace lookups
  - `using namespace` visibility
  - nested namespace definitions
  - namespace-qualified type/value references
- broader `ctest` only if the slice crosses more than parser namespace lookup

## Non-Goals

- no full semantic lexical-scope unification
- no removal of every canonical-name string bridge in one pass
- no repo-wide `std::string` to `TextId` migration
- no backend/shared semantic-id redesign

## Suggested Execution Decomposition

1. Convert namespace child registration to parent-context `TextId` child maps.
2. Make qualified-name namespace traversal consume `TextId` segments as the
   primary path.
3. Keep canonical string synthesis only as a compatibility/debug bridge.
4. Use the resulting namespace tree as a cleaner base for later parser binding
   and scope work, without forcing that work into this idea.
