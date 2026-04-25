# Sema Dual-Lookup Structured Identity Cleanup Runbook

Status: Active
Source Idea: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md

## Purpose

Move sema-owned lookup and validation state toward structured identity using
the parser cleanup's dual-lookup strategy: structured mirrors beside rendered
string tables, dual-write registrations, dual-read proof, then remove only
sema-owned string fallbacks that no longer serve AST/HIR bridge compatibility.

## Goal

Make sema lookup structured-first where stable AST/parser metadata is available,
while preserving rendered-name compatibility for HIR, diagnostics, consteval,
and codegen-facing surfaces.

## Core Rule

Do not require HIR to stop using rendered strings in this plan. Add structured
sema mirrors and mismatch proof first; remove legacy string lookup only after
equivalence is proven and the removed path is not a HIR/AST bridge.

## Read First

- `ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- `src/frontend/sema/canonical_symbol.hpp`

## Current Targets

- sema local scopes and variable lookup
- global symbols, functions, and overload sets
- enum constants and const-int bindings
- consteval function registry and interpreter locals
- sema-owned template/type binding lookup
- struct completeness and member/static-member lookup mirrors where structured
  identity already exists

## Non-Goals

- no HIR data-model cleanup
- no removal of `Node::name` or `TypeSpec::tag`
- no change to rendered diagnostics, mangled names, or link names unless proven
  equivalent
- no broad canonical-symbol rewrite before sema has structured input data
- no expectation downgrades to hide dual-lookup mismatches
- no testcase-shaped special cases for individual names

## Working Model

- Keep every existing rendered-string table populated during migration.
- Add structured-key mirrors only where sema can derive stable identity from
  AST/parser metadata such as `TextId`, qualified-name metadata, namespace
  context, or canonical parser identity.
- Dual-write declarations and bindings into the legacy and structured tables.
- Dual-read important lookup sites and compare structured vs string results
  before changing behavior.
- Treat rendered-name maps as bridge-required when HIR, consteval, diagnostics,
  or codegen still consume the spelling.
- Prefer mismatch diagnostics, assertions in proof paths, or focused tests over
  silent behavior changes.

## Execution Rules

- Keep each executor packet behavior-preserving unless this runbook explicitly
  allows a structured-first behavior change after proof.
- Update `todo.md` with packet progress and proof results; do not rewrite this
  plan for routine packet completion.
- For each code-changing step, run `cmake --build --preset default` plus the
  focused frontend/parser/sema CTest subset chosen by the supervisor.
- When a step touches consteval, template binding, function/global lookup, HIR
  edge behavior, or broad sema validation, use matching before/after CTest logs
  and regression-guard comparison before accepting the slice.
- Escalate to broader `ctest --test-dir build -j --output-on-failure` if a
  packet crosses into HIR or compile-time-engine behavior.
- Do not downgrade expectations or weaken supported-path tests to claim
  progress.

## Ordered Steps

### Step 1: Add Sema Structured Key Helpers

Goal: define the small structured identity helpers sema needs before adding
mirrors.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- `src/frontend/sema/canonical_symbol.hpp`

Actions:
- Inspect existing sema name rendering and lookup helper paths.
- Add helper types or functions that derive structured sema lookup keys from
  available AST/parser metadata without re-rendering names.
- Keep helper APIs optional/fallback-friendly for AST nodes that lack stable
  text ids or qualified-name metadata.
- Add mismatch-check helpers only where they can compare legacy and structured
  lookup results without changing user behavior.

Completion check:
- Key derivation helpers build cleanly.
- Existing string-keyed behavior remains intact.
- The next steps can dual-write maps without inventing per-map ad hoc key
  derivation.

### Step 2: Add Dual Local-Scope Symbol Maps

Goal: mirror scope-local bindings with structured `TextId` identity where
available.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`

Actions:
- Add structured local-scope maps beside the existing string-keyed scope maps.
- Dual-write local declarations, parameters, and block-scope symbols when the
  AST node exposes a usable text id.
- Dual-read variable lookup and compare structured and legacy results.
- Keep legacy fallback for nodes without stable structured identity.

Completion check:
- Local variable lookup remains behavior-compatible.
- Focused proof covers local shadowing, block-scope lookup, and global-vs-local
  name resolution.
- Mismatch handling reports or fails in a controlled proof path rather than
  silently changing semantics.

### Step 3: Add Dual Global, Function, and Overload Maps

Goal: make global/function/overload sema lookup structured-capable while
keeping rendered-name maps for bridge consumers.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- `src/frontend/sema/canonical_symbol.hpp`

Actions:
- Add structured sema symbol keys for global and function lookup when namespace
  context and base text id are available.
- Dual-write global variables and function declarations.
- Dual-write C++ overload sets and reference overload sets under structured
  owner/name identity.
- Use structured lookup first only where the AST carries qualified-name
  metadata and mismatch proof is in place.
- Preserve rendered-name maps for diagnostics, HIR handoff, and legacy call
  sites.

Completion check:
- Function/global lookup and overload resolution remain baseline-compatible.
- Focused proof covers namespace and class/member overload contexts.
- Matching before/after logs show no unexpected drift for the selected subset.

### Step 4: Add Dual Enum and Const-Int Binding Maps

Goal: mirror enum constants and const-int bindings with structured identity.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`

Actions:
- Add `TextId`-keyed mirrors for unqualified enum constants and const-int
  bindings.
- Add qualified structured keys where constants live in namespace or record
  contexts and sema has enough metadata.
- Dual-write global and local constant bindings.
- Dual-read case-label, static evaluation, and consteval lookup paths.
- Preserve string maps while consteval or HIR consumers still require rendered
  names.

Completion check:
- Focused proof covers enum constants, case-label constant evaluation, and
  const-int bindings.
- Consteval-facing behavior is unchanged except for controlled mismatch proof.

### Step 5: Add Dual Consteval Registry and Interpreter Bindings

Goal: give consteval sema paths structured mirrors where call and parameter
metadata support them.

Primary targets:
- `src/frontend/sema/consteval.cpp`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`

Actions:
- Add structured consteval function lookup where call AST nodes carry a
  structured callee identity.
- Add `TextId` mirrors for interpreter locals and NTTP bindings where source
  parameter text ids are available.
- Dual-read consteval function calls, parameter binding, assignment, and local
  lookup.
- Keep rendered string registries because the HIR compile-time engine still
  exposes string-based definitions.

Completion check:
- Focused proof covers consteval calls, nested consteval calls, parameter
  binding, assignment, and local lookup.
- Matching before/after logs show no unexpected drift for consteval subsets.

### Step 6: Add Dual Type-Binding Lookup

Goal: mirror sema-owned template/type substitutions without changing HIR-facing
`TypeSpec::tag` behavior.

Primary targets:
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`

Actions:
- Add structured binding keys for sema-owned template and type substitutions
  when parameter `TextId` or canonical template identity is available.
- Keep rendered `TypeSpec::tag` resolution intact for HIR and diagnostics.
- Compare structured and string type-binding resolution before changing lookup
  behavior.
- Avoid broad canonical-symbol rewrites unless the structured input data is
  already available at the sema boundary.

Completion check:
- Focused proof covers template type bindings and NTTP bindings in sema-owned
  checks.
- HIR-facing type tags remain stable.

### Step 7: Add Struct Completeness and Member Lookup Mirrors

Goal: add structured mirrors for record lookup only where sema can identify the
same record without HIR cleanup.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/sema/type_utils.hpp`

Actions:
- Preserve rendered struct/tag maps used by HIR and codegen.
- Add structured mirrors for complete structs/unions, field names, static
  members, and base tags only when parser/AST metadata can identify the same
  record without re-rendering.
- Dual-check struct completeness, field lookup, and static member lookup in
  focused cases.
- Leave full struct/tag identity cleanup to the later HIR-facing initiative.

Completion check:
- Focused proof covers struct completeness, member lookup, and static member
  lookup.
- No HIR-facing `TypeSpec::tag` behavior changes are required.

### Step 8: Remove Proven Sema-Owned String Fallbacks

Goal: remove only sema-owned legacy lookup fallback paths whose structured
mirrors are stable and not bridge-required.

Primary targets:
- the sema files touched by Steps 2-7

Actions:
- Inventory string fallbacks introduced or retained during this plan.
- Classify each as sema-owned removable, bridge-required, diagnostic-only,
  legacy-proof, or blocked-by-HIR.
- Remove only sema-owned fallbacks with matching structured proof and no
  downstream bridge requirement.
- Keep compatibility maps where HIR, diagnostics, consteval, or codegen still
  need rendered strings.

Completion check:
- Acceptance criteria from the source idea are met.
- Focused frontend/sema proof passes.
- Any touched broader baseline has matching before/after regression logs with
  no unexpected drift.
- Remaining string-keyed sema paths are documented in `todo.md` as bridge,
  diagnostic, legacy-proof, or HIR-blocked for the follow-on audit.

