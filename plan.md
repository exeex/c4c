# Parser Namespace TextId Context Tree

Status: Active
Source Idea: ideas/open/82_parser_namespace_textid_context_tree.md

## Purpose

Replace parser namespace lookup's canonical-string-driven path with a parent /
child namespace context tree keyed by `TextId` segments.

## Goal

Make namespace ownership and qualified-name traversal operate on structured
context plus `TextId` segments while preserving the existing namespace
push/pop registration model.

## Core Rule

Keep the work inside parser namespace lookup. Do not widen this into full
lexical-scope redesign, repo-wide `TextId` migration, or backend/HIR cleanup.

## Read First

- ideas/open/82_parser_namespace_textid_context_tree.md
- src/frontend/parser/parser.hpp
- src/frontend/parser/parser_state.hpp
- src/frontend/parser/parser_core.cpp
- nearby parser helper files that participate in qualified-name parsing and
  namespace lookup

## Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- nearby parser helper files that participate in namespace registration,
  qualified-name traversal, and using-directive visibility

## Non-Goals

- no full semantic lexical-scope unification
- no removal of every canonical-name string bridge in one pass
- no repo-wide `std::string` to `TextId` migration
- no sema, HIR, or backend identity redesign
- no testcase-shaped namespace shortcuts

## Working Model

- keep namespace push/pop as the ownership and lifetime surface
- treat qualified names as ordered `TextId` segments first, spelled strings
  second
- use parent-context child maps keyed by `TextId` instead of canonical
  `"A::B"`-style global string keys
- keep canonical string synthesis only as a compatibility/debug bridge while
  semantic lookup moves to the namespace tree

## Execution Rules

- prefer small behavior-preserving packets over broad parser rewrites
- keep namespace registration and lookup changes aligned with the existing
  parser state bundles
- preserve diagnostics and rendered spellings while lookup internals move to
  `TextId` traversal
- after each structural move, validate with
  `cmake --build build -j --target c4c_frontend c4cll`
- run focused parser/frontend tests covering namespace-qualified lookup,
  nested namespaces, and `using namespace` visibility before broadening
- escalate to broader `ctest` only when a packet crosses beyond parser
  namespace lookup or becomes a milestone checkpoint

## Validation

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests covering qualified namespace lookup,
  `using namespace`, nested namespace definitions, and namespace-qualified
  type/value references
- broader `ctest` when namespace lookup changes cross multiple parser
  subsystems or reach acceptance-ready milestones

## Step 1: Convert Namespace Child Registration To `TextId` Maps

Goal: store namespace children by parent context plus segment `TextId` rather
than canonical composed strings.

Primary targets:

- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser.hpp`

Actions:

- extend namespace state so named child lookup is driven by parent-context
  child maps keyed by `TextId`
- keep any required spelled/canonical name fields only as bridges for
  diagnostics and compatibility
- preserve anonymous-namespace handling and namespace push/pop behavior

Completion check:

- namespace child registration no longer depends on canonical composed-string
  keys as the primary identity path

## Step 2: Route Qualified Namespace Traversal Through `TextId` Segments

Goal: resolve qualified namespace names segment-by-segment through the context
tree.

Primary targets:

- `src/frontend/parser/parser_core.cpp`
- parser helper files that resolve `QualifiedNameRef` or namespace contexts

Actions:

- make qualified-name resolution consume `qualifier_text_ids` and
  `base_text_id` as the primary traversal path
- walk namespace contexts one segment at a time from the correct root/active
  scope instead of rebuilding `"A::B::C"` strings for lookup
- keep string spelling only as a bridge when diagnostics or existing helpers
  still need rendered names

Completion check:

- namespace traversal and lookup succeed through structured segment walking
  instead of canonical-string reconstruction

## Step 3: Contain Canonical String Fallbacks To Compatibility Helpers

Goal: demote canonical namespace strings to rendering/debug bridges rather
than semantic lookup keys.

Primary targets:

- `src/frontend/parser/parser_core.cpp`
- `src/frontend/parser/parser_state.hpp`
- nearby parser helper files that still synthesize canonical namespace names

Actions:

- isolate any remaining canonical-name helpers behind explicit compatibility or
  debug-only call sites
- confirm parser-visible behavior stays the same while semantic lookup uses the
  namespace tree
- avoid expanding the packet into unrelated binding-table or lexical-scope work

Completion check:

- canonical namespace strings remain available for diagnostics/debugging but
  are no longer the parser's primary namespace identity path

## Step 4: Lock The Route And Hand Off Later Scope Work Cleanly

Goal: end the runbook with a clear boundary between namespace-tree cleanup and
later parser scope work.

Primary targets:

- `todo.md`
- any parser files identified by prior steps as follow-on-only work

Actions:

- record the next narrow follow-on packet once namespace-tree lookup is stable
- keep later lexical-scope or wider parser binding work out of this runbook
- document any newly discovered separate initiative under `ideas/open/`
  instead of stretching this plan

Completion check:

- the runbook ends with namespace-tree lookup stabilized and a clear boundary
  for any later non-namespace follow-on
