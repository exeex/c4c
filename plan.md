# LIR String Constructor Deprecation Migration Runbook

Status: Active
Source Idea: ideas/open/760_lir_string_constructor_deprecation_migration.md
Depends On: ideas/closed/759_lir_typed_ref_enum_foundation.md

## Purpose

Use the typed LIR ref foundation to expose and migrate legacy string
construction in bounded groups while retaining legitimate runtime-text paths.

## Goal

Make closed-set construction enum-first and make every retained dynamic-text
boundary explicit, searchable, and locally proven.

## Core Rule

Do not remove generic string support or suppress warnings globally. First
classify each use as a closed-set migration or a legitimate runtime-text
boundary, then change one coherent group at a time.

## Read First

- `ideas/open/760_lir_string_constructor_deprecation_migration.md`
- `ideas/closed/759_lir_typed_ref_enum_foundation.md`
- `src/codegen/lir/types.hpp`
- current users of `LirTypeRef`, `LirBinaryOpcodeRef`, and
  `LirCmpPredicateRef`

## Current Targets

- `src/codegen/lir/types.hpp`
- `src/codegen/lir/hir_to_lir/stmt.cpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp` and nearby backend compatibility paths

## Non-Goals

- no repository-wide string-constructor rewrite
- no full typed representation for dynamic aggregate, vector, struct, or
  function spellings
- no wholesale HIR `TypeSpec` lowering rewrite
- no verifier weakening, expectation downgrade, or output-semantic change

## Working Model

Closed literal spellings migrate to enum construction. Dynamic text remains
supported but must use an explicit runtime-text factory or helper. Warnings
are an inventory signal and must not become undifferentiated noise.

## Execution Rules

1. Start with an inventory and a warning/proof strategy before adding an
   annotation or migrating callers.
2. Keep each implementation packet to one coherent call-site category.
3. Preserve dynamic runtime text behavior and document every deferred boundary.
4. Run a fresh build and focused proof for every accepted code-changing packet.
5. Escalate to broader LIR/frontend/backend coverage when a shared path changes.

## Ordered Steps

### Step 1 - Inventory constructor uses and choose warning proof

Goal: classify current relevant string construction before changing behavior.

Primary targets:

- `src/codegen/lir/types.hpp`
- the initial-runtime-inventory paths named by the source idea

Actions:

- enumerate direct string construction and string-only factory uses for LIR
  type, opcode, and predicate refs
- classify each as closed-set literal, typed-lowering candidate, or legitimate
  dynamic runtime text
- choose a bounded deprecation-warning form and a compile/test proof that
  exposes the intended inventory without making unrelated compilation fail
- record the first coherent migration group in `todo.md`; do not migrate it in
  this audit packet

Completion check:

- the next packet has a named call-site group, an explicit runtime-text
  boundary list, and a warning/proof strategy that does not require a broad
  migration.

### Step 2 - Add a narrow warning and explicit runtime-text boundary

Goal: make one constructor family’s legacy text path visible without breaking
legitimate dynamic construction.

Actions:

- introduce the selected deprecation annotation or explicit runtime-text
  factory name in the narrowest appropriate LIR ref surface
- route only the classified dynamic boundary needed by this packet through the
  explicit path
- keep the change source-compatible outside the selected packet

Completion check:

- warnings identify real selected migration sites and the retained dynamic path
  is explicit and still compiles.

### Step 3 - Migrate one closed-set call-site group

Goal: replace a small coherent set of literal closed-set uses with enum
construction.

Actions:

- migrate only the group named by the Step 1 inventory
- retain or add focused coverage of construction, printing, and lowering for
  that group
- document any deferred runtime-text use at the nearest appropriate boundary

Completion check:

- the selected closed-set uses no longer rely on a generic string constructor,
  while dynamic text behavior is unchanged.

### Step 4 - Repeat bounded groups and track remaining boundaries

Goal: make incremental progress without turning the route into a broad rewrite.

Actions:

- select the next inventory category only after the prior packet is proven
- migrate literal uses or name explicit runtime-text helpers as appropriate
- keep remaining warnings classified rather than silenced

Completion check:

- each accepted group has local proof and remaining warning sites are an
  auditable, intentionally deferred list.

### Step 5 - Validate migration state and choose follow-up

Goal: verify accepted packets and decide whether the source criteria are met.

Actions:

- run fresh build and focused LIR/frontend/backend tests for the changed paths
- inspect for verifier relaxation, expectation downgrade, or unrelated rewrites
- document any remaining legitimate runtime-text boundaries and determine
  whether a separate successor is required

Completion check:

- the supervisor has coherent evidence for closure or an explicit in-scope
  repair/successor route.
