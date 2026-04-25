# Sema Dual-Lookup Structured Identity Cleanup

Status: Open
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [95_parser_dual_lookup_structured_identity_cleanup.md](/workspaces/c4c/ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md)

## Goal

Move sema-owned lookup and validation state toward structured identity using
the same dual-lookup strategy as the parser cleanup: add structured lookup
beside existing rendered-string lookup, prove the two agree, and only remove
string-keyed sema tables when the current sema layer can do so without
requiring a HIR data-model rewrite.

HIR cleanup is intentionally out of scope for this idea. HIR still has its own
string-keyed module, template, consteval, and codegen-facing identity surfaces;
those will be discussed under a separate follow-on initiative.

## Why This Idea Exists

The parser has been moving toward `TextId`, `NamePathId`, and
`QualifiedNameKey` for semantic lookup. Sema still has many string-keyed maps
because its inputs and downstream consumers are still mostly rendered AST/HIR
names:

- `Node::name`
- `TypeSpec::tag`
- rendered function and overload names
- rendered struct/global/enum names
- consteval and template binding names

Some of those strings are bridge surfaces and should remain until HIR has its
own structured identity plan. But sema also owns lookup tables that can start
carrying structured IDs now, especially where parser already annotates AST
nodes with `TextId`, qualifier text ids, namespace context ids, or where sema
can derive a stable structured key beside the existing spelling.

This idea exists to make sema identity cleanup incremental and baseline-safe.

## Main Objective

Add sema-side structured lookup mirrors for currently string-keyed semantic
tables, then migrate sema-owned queries to structured-first / legacy-checked
behavior where structured identity is available.

The governing strategy is:

1. keep the rendered-string table populated
2. add the structured key table beside it
3. dual-write registrations
4. dual-read important lookup sites
5. report or test mismatches before changing behavior
6. remove only sema-owned string fallbacks that no longer serve HIR/AST bridge
   compatibility

## Primary Sema Scope

- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- `src/frontend/sema/canonical_symbol.hpp`
- focused frontend/sema/parser tests that exercise the same semantic paths

## Sema-Owned Cleanup Targets

### A. Scoped Local Symbol Tables

`SemaValidator` stores local scopes as string-keyed maps. Parser AST nodes
already often carry `unqualified_text_id` and qualifier metadata.

Expected direction:

- add a `TextId`-keyed local symbol mirror for scope-local bindings where the
  AST node provides a usable text id
- keep the string scope map as the compatibility source during migration
- dual-write local declarations, parameters, and block-scope symbols
- dual-read variable lookup and compare structured vs string results
- keep fallback to string for AST nodes without stable text ids

### B. Global Symbols, Functions, and Overload Sets

Sema currently tracks globals, function signatures, C++ overload sets, and
reference overload sets by rendered name.

Expected direction:

- add structured sema symbol keys for function/global lookup when namespace
  context and base `TextId` are available
- dual-write global variables and function declarations
- dual-write C++ overload sets under structured owner/name identity
- keep rendered-name maps for diagnostics, HIR handoff, and legacy call sites
- use structured lookup first for AST nodes that carry qualified-name metadata
  and compare with string lookup while migrating

### C. Enum and Const-Int Bindings

Sema and sema-adjacent consteval paths use string-keyed enum and const-int
binding maps.

Expected direction:

- add `TextId`-keyed mirrors for unqualified enum constants and const-int
  bindings
- add qualified structured keys where enum constants live in a namespace or
  record context
- dual-write global and local constant bindings
- dual-read case-label, static evaluation, and consteval lookup paths
- preserve string maps while HIR/consteval consumers still require them

### D. Consteval Function Registry and Interpreter Locals

`consteval` evaluation uses rendered function names, local names, type
bindings, and NTTP bindings.

Expected direction:

- add structured consteval function lookup where call AST nodes carry a
  structured callee identity
- add `TextId` mirrors for interpreter locals and NTTP bindings where source
  parameter text ids are available
- keep string maps because HIR compile-time engine still exposes string
  registries
- dual-read consteval function calls, parameter binding, assignment, and local
  lookup during migration
- do not change user-facing consteval diagnostics except to add optional
  mismatch diagnostics in debug/proof paths

### E. Type Binding and Canonical Symbol Helpers

Template type bindings and canonical symbol construction still rely on
`TypeSpec::tag` and rendered template parameter names.

Expected direction:

- add structured binding keys for sema-owned template/type substitutions when
  parameter `TextId` or canonical template identity is available
- keep rendered `TypeSpec::tag` behavior intact for HIR and diagnostics
- compare structured and string type-binding resolution before changing lookup
  behavior
- avoid broad canonical-symbol rewrites unless the structured data is already
  present in sema inputs

### F. Struct Completeness and Member Lookup Mirrors

Sema tracks complete structs/unions, field names, static members, and base tags
by rendered struct names.

Expected direction:

- do not remove rendered struct/tag maps in this idea, because HIR and codegen
  still depend heavily on rendered `TypeSpec::tag`
- add structured mirrors only where parser/AST metadata can identify the same
  record without re-rendering
- dual-check struct completeness, field lookup, and static member lookup in
  focused cases
- leave full struct/tag identity cleanup to the later HIR-facing initiative

## HIR Boundary

HIR is deliberately not cleaned in this idea.

The following are acknowledged downstream string consumers and should remain
stable until a HIR-specific plan exists:

- HIR module maps keyed by rendered struct/function/global names
- compile-time engine template and consteval definition registries
- HIR template instantiation work queues and instance maps
- HIR struct layout maps keyed by `TypeSpec::tag`
- codegen/link-name surfaces

This sema idea may add structured metadata or dual-lookup adapters at the sema
edge, but it must not require HIR to stop using string keys.

## Validation Strategy

Baseline stability is part of the work.

At minimum for each sema-only slice:

- run `cmake --build --preset default`
- run the focused frontend/parser/sema CTest subset that covers the touched
  path

When a slice touches consteval, template binding, or function/global lookup:

- generate matching before/after logs with the same CTest command
- compare them with `c4c-regression-guard`
- escalate to broader `ctest --test-dir build -j --output-on-failure` if the
  touched path crosses into HIR or compile-time-engine behavior

Focused proof should include:

- local shadowing and block-scope lookup
- global vs local name resolution
- enum constants and case-label constant evaluation
- const-int bindings
- consteval function calls and nested consteval calls
- template type/NTTP bindings in sema-owned checks
- overload set lookup in namespace and class/member contexts
- struct completeness, member lookup, and static member lookup

## Non-Goals

- no HIR data-model cleanup in this idea
- no removal of `Node::name` or `TypeSpec::tag`
- no change to rendered diagnostics, mangled names, or link names unless
  explicitly proven equivalent
- no broad canonical-symbol rewrite before sema has structured input data
- no expectation downgrades to hide dual-lookup mismatches
- no testcase-shaped special cases for individual names

## Suggested Execution Decomposition

1. Add sema structured key helpers that can derive identity from AST
   `TextId`/qualified-name metadata where available.
2. Add dual local-scope symbol maps and dual-read local variable lookup.
3. Add dual global/function/overload maps and structured-first lookups.
4. Add dual enum and const-int binding maps.
5. Add dual consteval function registry and interpreter local/NTTP binding
   lookup where text ids are available.
6. Add dual type-binding lookup for sema-owned template substitution paths.
7. Add structured mirrors for struct completeness/member lookup only where
   available without changing HIR-facing `TypeSpec::tag` behavior.
8. Remove sema-owned string fallbacks only after matching proof is stable and
   regression guard shows no baseline drift.

## Acceptance Criteria

- Sema has structured lookup mirrors for local symbols, globals/functions, and
  enum/const-int bindings where AST metadata provides stable identity.
- Important sema lookup paths are dual-read with mismatch proof before behavior
  changes.
- Consteval lookup has structured mirrors where possible while preserving HIR
  compile-time-engine string compatibility.
- Struct/tag and template identity cleanup is clearly separated from HIR
  cleanup and does not force HIR changes.
- Focused sema/frontend proof passes.
- Any touched broader baseline has matching before/after regression logs with
  no unexpected drift.
