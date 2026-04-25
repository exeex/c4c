# HIR To LIR Struct Layout Lookup By StructNameId

Status: Active
Source Idea: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md

## Purpose

Move selected HIR-to-LIR struct layout lookup paths toward
`StructNameId`-keyed `LirStructDecl` lookup while keeping legacy rendered-tag
lookup as the behavior-preserving fallback.

Goal: selected aggregate lowering paths prefer structured layout lookup when a
known `StructNameId` and matching `LirStructDecl` are available, with parity
observation against the legacy `TypeSpec::tag` path.

Core Rule: do not change emitted LLVM output, aggregate layout, ABI behavior,
or high-level HIR record semantics in this plan.

## Read First

- `ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md`
- `ideas/closed/106_shared_struct_name_table_for_lir_type_identity.md`
- `ideas/closed/107_lir_struct_name_id_type_ref_mirror.md`
- `ideas/closed/108_lir_struct_name_id_for_globals_functions_and_externs.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/codegen/lir/verify.cpp`
- focused aggregate, field access, initializer, and call-lowering tests under
  `tests/`

## Current Targets

- Struct size and alignment helpers used during HIR-to-LIR lowering.
- GEP, lvalue, and nested field-chain resolution.
- Constant initializer aggregate and field lookup.
- Byval and byref aggregate call handling.
- Fallback paths that still use `TypeSpec::tag` and `Module::struct_defs`.
- Parity or mismatch observation between structured `LirStructDecl` lookup and
  legacy rendered-tag lookup.

## Non-Goals

- Do not remove `TypeSpec::tag` lookup or `Module::struct_defs` fallback.
- Do not make `LirStructDecl` the printer authority.
- Do not change emitted LLVM type declarations, field order, sizes, alignment,
  calling convention, or aggregate initializer text.
- Do not add testcase-shaped matching for specific known failures.
- Do not erase HIR record semantics into LIR beyond already-lowered field
  layout.

## Working Model

- The legacy path derives layout from rendered struct tags and
  `Module::struct_defs`.
- The structured path should first resolve a known HIR struct type to
  `StructNameId`, then use the LIR module's `StructNameId -> LirStructDecl`
  mirror.
- Structured lookup is authoritative only for the local lookup attempt when it
  is known and matches the legacy result; legacy lookup remains the fallback
  and compatibility check.
- Mismatch observation should identify the type, lookup site, and legacy-vs-
  structured result clearly enough for later printer-authority work.

## Execution Rules

- Keep each step behavior-preserving and independently provable.
- Prefer shared lookup helpers over per-call-site ad hoc conversions.
- Add structured lookup only where the source type already has a known
  `StructNameId`.
- Compare structured and legacy lookup results before trusting the structured
  result in a path that previously depended on rendered text.
- Update verifier or focused debug/proof hooks after data exists; do not make
  checks outrun lowering support.
- Use focused tests for aggregate layout, nested field access, constant
  initializers, and aggregate call handling; escalate to broader backend or LIR
  validation after multiple lookup paths have landed.

## Ordered Steps

### Step 1: Audit Existing Layout Lookup Surfaces

Goal: identify the HIR-to-LIR helpers and call sites that still resolve struct
layout through rendered tags.

Primary targets:

- `src/codegen/lir/hir_to_lir/`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/verify.cpp`

Actions:

- Trace size, alignment, field lookup, GEP, initializer, and aggregate call
  helpers that call through `TypeSpec::tag`, `struct_defs`, or equivalent
  rendered-name maps.
- Identify where the lowered HIR type already has enough information to recover
  or carry `StructNameId`.
- Separate direct struct lookup sites from sites that need a prior type-ref or
  lowering helper before structured lookup is possible.
- Record the first bounded implementation packet in `todo.md`; do not edit the
  source idea for routine audit findings.

Completion check:

- The executor can name the exact lookup helper or call-site family for Step 2
  and describe the legacy fallback that must remain in place.

### Step 2: Introduce A Shared Structured Layout Lookup Helper

Goal: provide one behavior-preserving helper that tries `StructNameId` layout
lookup before legacy rendered-tag lookup.

Primary targets:

- HIR-to-LIR type/layout helper files under `src/codegen/lir/hir_to_lir/`
- `LirModule::struct_decls` and legacy `struct_defs` access points
- `src/codegen/lir/verify.cpp` if parity checking belongs there

Actions:

- Add or extract a helper that accepts the current type context and can attempt
  `StructNameId -> LirStructDecl` lookup when the ID is known.
- Keep the legacy rendered-tag lookup as the fallback result.
- Add mismatch observation for field count, field type text, size/alignment, or
  whichever layout facts the call site consumes.
- Keep helper output narrow enough that existing callers do not need a broad
  rewrite.

Completion check:

- The helper can be used by one existing layout path without changing emitted
  LLVM or deleting the old lookup path.

### Step 3: Prefer Structured Lookup For Size And Alignment Helpers

Goal: route struct size and alignment queries through the shared helper where a
known `StructNameId` exists.

Primary targets:

- HIR-to-LIR size and alignment helpers
- Focused aggregate layout tests

Actions:

- Replace direct rendered-tag lookups in size and alignment paths with the
  shared helper.
- Preserve legacy behavior for anonymous, unsupported, or not-yet-structured
  aggregate types.
- Add focused proof that aggregate layout output is unchanged.

Completion check:

- Size and alignment paths prefer structured layout when available, fall back
  to legacy lookup when needed, and pass focused aggregate layout tests without
  output drift.

### Step 4: Prefer Structured Lookup For Field Chains And GEP Lowering

Goal: use structured layout lookup for direct and nested field resolution where
the struct identity is known.

Primary targets:

- GEP lowering
- lvalue field-chain resolution
- nested struct field lookup helpers
- focused field access tests

Actions:

- Convert direct field lookup call sites to use the shared layout helper.
- Handle nested field chains one struct identity at a time; do not invent
  named-case shortcuts.
- Preserve legacy rendered-tag fallback and mismatch observation.
- Prove direct and nested field access emit unchanged LLVM.

Completion check:

- Field-chain and GEP lowering can resolve known structs through
  `StructNameId`, while nearby fallback cases still behave as before.

### Step 5: Prefer Structured Lookup For Constant Initializers

Goal: use structured lookup for aggregate initializer field ordering and nested
initializer resolution where a known struct identity is available.

Primary targets:

- constant initializer lowering under `src/codegen/lir/hir_to_lir/`
- aggregate and nested initializer tests

Actions:

- Route initializer aggregate lookup through the shared helper.
- Preserve field order, default value handling, and rendered initializer text.
- Add parity checks or focused assertions for nested struct initializer cases.

Completion check:

- Struct constant initializers prefer structured layout where available and
  focused initializer tests show no LLVM output drift.

### Step 6: Prefer Structured Lookup For Aggregate Call Handling

Goal: extend structured lookup to byval, byref, and aggregate call paths that
need layout facts.

Primary targets:

- HIR-to-LIR call lowering
- aggregate argument and return handling helpers
- focused call and ABI tests

Actions:

- Identify byval/byref layout queries that consume struct field or size facts.
- Route eligible queries through the shared helper while preserving ABI and
  call text.
- Keep unsupported or identity-missing cases on the legacy path.
- Prove aggregate call tests remain behavior-preserving.

Completion check:

- Aggregate call handling uses structured layout for known structs without
  changing ABI classification, byval/byref behavior, or emitted call text.

### Step 7: Validate Parity And Prepare For The Printer Audit

Goal: prove the selected lookup migration is stable and ready for the idea 110
readiness audit.

Primary targets:

- focused aggregate, field access, initializer, and aggregate call tests
- broader LIR/backend validation chosen by the supervisor
- mismatch observation added by this plan

Actions:

- Run fresh build or compile proof for each implementation packet.
- Run focused tests after each migrated lookup family.
- After the selected families are complete, run the broader LIR/backend subset
  or full validation directed by the supervisor.
- Leave printer-authority decisions and rendered-string classification to idea
  110.

Completion check:

- Acceptance criteria from the source idea are satisfied: selected lookup paths
  prefer `StructNameId` when known, legacy fallback remains available, parity
  observation exists, and focused tests pass without output drift.
