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
supported but must use an explicit runtime-text factory or helper. A global
constructor deprecation is deferred because its trial warned at unrelated
callers; use local proof and explicit construction boundaries instead.

## Execution Rules

1. Start with an inventory and a local proof strategy before adding an
   annotation or migrating callers; do not retry global constructor
   deprecation while it produces unrelated warning noise.
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

### Step 2 - Make the va_list runtime-text boundary explicit

Goal: migrate the first closed va_list literals while making its generated
array/padding text explicit, without a global deprecation annotation.

Primary targets:

- `src/codegen/lir/hir_to_lir/stmt.cpp`
- the narrow `LirTypeRef` construction surface needed for an explicit
  runtime-text boundary

Actions:

- introduce an explicit runtime-text factory or helper without deprecating the
  generic constructor globally
- route only `build_type_decls`' generated array/padding text through that
  explicit boundary
- migrate only `build_type_decls`' `"i32"` and `"ptr"` va_list literals to
  the existing enum construction path
- build and run the focused LIR call-type proof for this one source function

Completion check:

- the selected literals are enum-constructed, generated runtime text is
  explicit, and local proof passes without unrelated warning inventory noise.

### Step 3 - Migrate a subsequent closed-set call-site group

Goal: replace a small coherent set of literal closed-set uses with enum
construction.

Actions:

- choose and migrate only the next inventory group after Step 2 proof
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

### Step 6 - Repair the targeted warning-inventory route

Goal: satisfy the source idea's warning-inventory criterion without reviving
the rejected global `const char*` deprecation experiment.

Accepted evidence before this repair:

- `a1d6fd79a` added the explicit `build_type_decls` runtime-text boundary and
  migrated its va_list literals; fresh build plus `^frontend_lir_call_type_ref$`
  passed.
- `c4c021559` migrated the static no-expression return literals; fresh build
  plus `^frontend_hir_tests$` passed.
- `896779447` migrated the selected-byval pointer-authority literals; fresh
  build plus `^backend_lir_selected_pointer_authority$` passed.
- The final fresh build and `^(frontend_lir_|backend_lir_)` focused run passed
  6/6, and the accepted full baseline was 3034/3034 passing.

Actions:

- design and prove a narrowly scoped, targeted deprecation-warning inventory
  boundary for one remaining construction category; it must not emit the
  widespread unrelated warnings produced by the rejected global constructor
  annotation
- use that boundary to classify the selected remaining sites as closed-set
  enum candidates or legitimate explicit runtime-text paths
- record the resulting auditable remaining-boundary inventory and either
  continue the next bounded migration group or return to this close gate with
  evidence that the source criteria are satisfied

Completion check:

- a targeted warning inventory identifies real remaining construction sites
  without blocking unrelated compilation, and every classified retained
  runtime-text boundary is searchable and documented; otherwise the plan
  names the exact in-scope repair still required.
