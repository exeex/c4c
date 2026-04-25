# LIR StructNameId TypeRef Mirror

Status: Active
Source Idea: ideas/open/107_lir_struct_name_id_type_ref_mirror.md

## Purpose

Extend the `StructNameId` dual-track strategy from LIR struct declarations into
struct-valued `LirTypeRef` instances.

## Goal

Make struct and union LIR type references capable of carrying a
`StructNameId` mirror beside their existing rendered LLVM type string, while
keeping the rendered string as printer authority and preserving emitted LLVM.

## Core Rule

Do not remove or demote rendered type strings in this plan. Add structured
`StructNameId` mirrors beside them, prove shadow-render parity where both
forms exist, and keep printer output unchanged.

## Read First

- `ideas/open/107_lir_struct_name_id_type_ref_mirror.md`
- `ideas/closed/106_shared_struct_name_table_for_lir_type_identity.md`
- `src/shared/struct_name_table.hpp`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/lir_printer.cpp`

## Current Targets

- Add optional `StructNameId` metadata to `LirTypeRef` or the narrow local
  equivalent that preserves existing construction patterns.
- Add struct-aware factories or constructors that dual-write rendered text and
  `StructNameId` when the ID is known.
- Thread the mirror through HIR-to-LIR type-ref creation paths that already
  know the interned struct name.
- Add verifier parity checks for struct type refs where both forms are present.
- Keep all existing `llvm_ty`, `llvm_alloca_ty`, `TypeSpec::tag`, and rendered
  string paths intact.

## Non-Goals

- Do not make `StructNameId` the printer authority.
- Do not remove `TypeSpec::tag`, `llvm_ty`, `llvm_alloca_ty`, or legacy
  rendered type strings.
- Do not rewrite global/function/extern signature surfaces beyond what is
  necessary to keep `LirTypeRef` construction coherent; those belong to idea
  108.
- Do not move layout lookup from `TypeSpec::tag` to `StructNameId`; that
  belongs to idea 109.
- Do not change emitted LLVM type names, declaration text, function/global
  signatures, or ABI behavior.

## Working Model

Idea 106 created the structured struct declaration mirror:

```text
StructNameId -> LirStructDecl -> fields/layout
legacy type_decls -> unchanged printer emission
```

This plan extends that mirror to type references:

```text
LirTypeRef.rendered -> unchanged printer/emission text
LirTypeRef.struct_name_id -> optional structured mirror for struct/union refs
```

The rendered string remains the compatibility and printer authority until a
later readiness audit proves enough coverage to switch.

## Execution Rules

- Keep every code step behavior-preserving.
- Prefer local construction helpers over broad rewrites of LIR call sites.
- Dual-write only where a reliable `StructNameId` is already available.
- Preserve legacy behavior when no `StructNameId` is known.
- Add parity observation before trusting the structured mirror.
- Record any string-only type-ref surfaces in `todo.md` instead of expanding
  this plan into idea 108 or 109 scope.

## Steps

### Step 1: Inspect LirTypeRef Construction And Struct Type Spelling

Goal: identify the narrow type-ref construction points that can safely carry a
`StructNameId` mirror.

Primary targets:
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- helper surfaces that render `TypeSpec::tag` or LLVM struct names into
  `%struct.*` strings

Actions:
- Inspect `LirTypeRef` fields, constructors, and helper functions.
- Find HIR-to-LIR paths that create struct-valued type refs or rendered LLVM
  type strings.
- Identify which paths already have access to the module `StructNameTable` or
  an interned `StructNameId`.
- Record any major string-only surfaces in `todo.md` as watchouts for later
  ideas.

Completion check:
- The first implementation packet has a concrete list of type-ref construction
  points and a minimal edit target for adding the mirror.

### Step 2: Add Optional StructNameId Metadata To LirTypeRef

Goal: make `LirTypeRef` able to carry a structured struct-name mirror without
changing existing rendered text behavior.

Primary targets:
- `src/codegen/lir/types.hpp`
- direct construction sites that need compile fixes

Actions:
- Add optional `StructNameId` metadata to `LirTypeRef`, or a narrow companion
  wrapper if that better matches local style.
- Preserve the rendered LLVM type string as the existing authority.
- Add convenience checks for whether a type ref carries a valid struct mirror.
- Keep non-struct and unknown-struct type refs behavior-compatible.

Completion check:
- The project builds with existing type-ref construction behavior preserved.

### Step 3: Add Struct-Aware TypeRef Factories

Goal: centralize dual-writing of rendered LLVM type text and `StructNameId`.

Primary targets:
- `src/codegen/lir/types.hpp`
- nearby HIR-to-LIR type helper functions

Actions:
- Add factories or helper constructors for struct and union type refs.
- Render the same legacy LLVM type string as before.
- Attach `StructNameId` only when the ID is known.
- Keep fallback constructors for legacy string-only type refs.

Completion check:
- New struct-aware helpers produce exactly the same rendered string as the
  legacy path while also carrying the structured mirror.

### Step 4: Thread StructNameId Through HIR-To-LIR TypeRef Creation

Goal: populate struct-aware type refs where HIR-to-LIR already has enough
structured identity.

Primary targets:
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- shared LLVM type rendering helpers used by HIR-to-LIR

Actions:
- Use the `StructNameTable` introduced by idea 106 when creating struct or
  union type refs.
- Replace local string-only construction with the new struct-aware helper only
  where the same LLVM type spelling can be preserved.
- Keep legacy string-only fallback paths intact.
- Do not change global/function/extern signature authority as part of this
  step.

Completion check:
- Representative struct-valued type refs carry `StructNameId` mirrors, and
  emitted LLVM remains unchanged.

### Step 5: Add TypeRef Parity Verification

Goal: prove the structured mirror shadow-renders to the same type spelling as
the legacy rendered string.

Primary targets:
- `src/codegen/lir/verify.cpp`
- any printer-adjacent type rendering helpers needed for parity

Actions:
- Shadow-render `StructNameId` through `StructNameTable`.
- Compare the shadow-rendered struct type spelling with the legacy rendered
  `LirTypeRef` text when both are present.
- Report missing or mismatched parity through existing verifier diagnostics.
- Keep the printer using rendered type strings.

Completion check:
- Verifier parity checks catch mismatched struct type-ref mirrors without
  affecting successful legacy output.

### Step 6: Focused Validation

Goal: prove the type-ref mirror is behavior-preserving and ready for dependent
ideas.

Primary targets:
- build and focused test commands chosen by the supervisor

Actions:
- Run `cmake --build --preset default`.
- Run focused LIR/codegen coverage for ordinary structs, unions, template
  struct/class instantiations, nested struct fields, aggregate loads/stores,
  and initializer paths that exercise struct-valued type refs.
- Escalate to broader CTest if the diff touches shared type rendering,
  verifier behavior, or broad HIR-to-LIR lowering helpers.

Completion check:
- Focused proof passes with unchanged emitted LLVM behavior.
- Remaining string-only type-ref surfaces are recorded as follow-up scope for
  idea 108, idea 109, or a new open idea if they are separate.

## Close Criteria

- Struct/union `LirTypeRef` instances can carry `StructNameId` beside rendered
  text.
- Existing rendered type strings remain present and authoritative for printing.
- HIR-to-LIR dual-writes known struct type refs without output drift.
- Verifier or focused tests prove `StructNameId` rendering matches legacy type
  strings where both exist.
- No legacy rendered type string path is removed.
- Validation passes without expectation downgrades or testcase-shaped
  shortcuts.
