# LIR Typed Ref Enum Foundation Runbook

Status: Active
Source Idea: ideas/open/759_lir_typed_ref_enum_foundation.md
Follow-Up: ideas/open/760_lir_string_constructor_deprecation_migration.md

## Purpose

Add typed enum/id carriers and enum-first constructors for closed LIR reference
wrappers without forcing the still-legitimate runtime text paths to migrate in
the same slice.

## Goal

Make closed builtin LIR type and opcode/predicate identity queryable through
typed ids after construction, with string rendering preserved only as an output
and compatibility boundary.

## Core Rule

Do not delete generic runtime text support in this idea. Runtime type text from
HIR `TypeSpec` lowering, struct layout, parsed call args, extern signatures,
and verifier compatibility must continue to compile while closed-set values
gain typed enum authority.

## Read First

- `ideas/open/759_lir_typed_ref_enum_foundation.md`
- `ideas/open/760_lir_string_constructor_deprecation_migration.md`
- `src/codegen/lir/types.hpp`
- representative current users of `LirTypeRef`, `LirBinaryOpcodeRef`, and
  `LirCmpPredicateRef`

## Current Targets

- `src/codegen/lir/types.hpp`
- nearby focused tests that exercise LIR type/ref construction, printing,
  verification, and lowering

## Non-Goals

- no removal of `LirTypeRef` runtime string construction
- no broad migration of existing string call sites
- no rewrite of HIR `TypeSpec` to LLVM type rendering
- no aggregate/vector/struct/function type tree replacement
- no verifier relaxation, BIR semantic change, LLVM IR emission change, or
  expectation downgrade

## Working Model

Closed-set spellings should have a typed enum/id carrier. Rendered text remains
available for printing and LLVM output, but semantic queries for closed builtin
types and closed opcode/predicate wrappers should prefer the typed carrier.

Dynamic type text remains explicit compatibility input for this foundation
slice. Idea 760 owns the later `[[deprecated]]` inventory and migration of
string call sites.

## Execution Rules

1. Keep the change centered on `src/codegen/lir/types.hpp`.
2. Add enum constructors before changing query behavior.
3. Preserve existing string constructors and runtime text behavior.
4. Convert only local implementation details needed to make the enum carrier
   authoritative for closed-set values.
5. Add or update focused coverage near the changed typed ref behavior.
6. Use build plus focused LIR/frontend/backend tests as proof; escalate broader
   validation only if shared behavior changes beyond the planned surface.

## Ordered Steps

### Step 1 - Inspect existing ref wrapper authority

Goal: identify the closed-set wrappers and current string-parsing authority in
`src/codegen/lir/types.hpp`.

Primary target:

- `src/codegen/lir/types.hpp`

Actions:

- inspect `LirTypeRef`, `LirBinaryOpcodeRef`, `LirCmpPredicateRef`, and any
  neighboring closed-set ref classes in the same header
- identify which classes already have enums and which still need enum/id
  carriers
- identify which typed queries currently reparse rendered text and which can
  safely prefer enum/id state

Completion check:

- the implementation scope is limited to closed-set ref construction and query
  authority; dynamic runtime text boundaries are left intact.

### Step 2 - Add enum/id construction for `LirTypeRef`

Goal: give `LirTypeRef` an authoritative enum/id path for closed builtin types.

Primary target:

- `src/codegen/lir/types.hpp`

Actions:

- add a builtin LIR type enum/id covering void, pointer, fixed integer widths,
  and floating scalar types
- add direct `LirTypeRef` construction from that enum/id
- retain string/runtime constructors for dynamic type spellings
- preserve rendered text generation for output boundaries
- expose typed query accessors so closed builtin identity does not require
  reparsing `str()`

Completion check:

- existing dynamic spellings still compile, while enum-constructed builtin
  refs can answer kind/width/identity through typed state.

### Step 3 - Extend the enum-first pattern to related closed-set refs

Goal: make nearby closed-set wrappers use the same enum-first construction
model where practical.

Primary target:

- `src/codegen/lir/types.hpp`

Actions:

- review wrappers such as `LirBinaryOpcodeRef` and `LirCmpPredicateRef`
- add or normalize enum constructors and typed accessors where the class has a
  closed semantic set
- keep text construction available for legacy/runtime compatibility
- avoid introducing deprecation warnings in this idea; that belongs to 760

Completion check:

- closed-set wrappers in `types.hpp` have a consistent enum-first API without
  breaking current callers.

### Step 4 - Add focused proof for typed authority

Goal: prove enum/id construction is real authority, not decorative metadata.

Primary targets:

- existing LIR type/ref tests or the nearest frontend/backend focused tests

Actions:

- add or update focused tests for enum-constructed `LirTypeRef` builtin kinds
  and integer widths
- cover string/runtime construction compatibility for at least one dynamic
  text form that must remain supported
- cover related closed-set wrappers when their constructor/query behavior
  changes

Completion check:

- tests would fail if closed builtin queries only worked by reparsing rendered
  text or if runtime text compatibility was accidentally removed.

### Step 5 - Validate and hand off to 760

Goal: finish 759 with a coherent proof and an explicit successor route.

Actions:

- run a fresh build
- run focused LIR/frontend/backend tests that cover `LirTypeRef` construction,
  printing, verification, and lowering
- inspect the diff for accidental verifier relaxation, expectation downgrades,
  or broad call-site migration
- leave `ideas/open/760_lir_string_constructor_deprecation_migration.md` as the
  named follow-up after 759 closes

Completion check:

- 759 acceptance criteria are satisfied, no runtime string constructor removal
  occurred, and the supervisor can move next to 760 after closure.
