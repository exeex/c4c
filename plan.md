# Shared StructNameId Table For LIR Type Identity

Status: Active
Source Idea: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md

## Purpose

Introduce a shared `StructNameId` / `StructNameTable` and wire it into LIR plus
HIR-to-LIR as a structured mirror for LLVM struct type declarations.

## Goal

Create a dual-track LIR struct type identity path where existing rendered
`type_decls` remain the emission authority while a structured
`StructNameId -> LirStructDecl` mirror is dual-written and parity-checked.

## Core Rule

Do not remove or demote the legacy rendered `type_decls` path in this plan.
Add the structured path beside it, prove exact output parity, and keep emitted
LLVM unchanged.

## Read First

- `ideas/open/106_shared_struct_name_table_for_lir_type_identity.md`
- `src/shared/text_id_table.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/verify.cpp`

## Current Targets

- Add shared `StructNameId` and `StructNameTable`.
- Attach `StructNameTable` to `LirModule` using the existing shared
  `TextTable`.
- Add a minimal structured LIR struct declaration mirror beside rendered
  `type_decls`.
- Populate that mirror during HIR-to-LIR type declaration generation.
- Add parity/shadow-render proof without changing printer output.

## Non-Goals

- Do not model C++ class, inheritance, method ownership, or template semantic
  identity in LIR.
- Do not carry `HirRecordOwnerKey` as the primary LIR type identity.
- Do not remove `TypeSpec::tag` from HIR.
- Do not remove or rewrite rendered `type_decls` as the first slice.
- Do not rewrite all `LirTypeRef` type strings.
- Do not change emitted LLVM type names, function/global link names, or ABI
  behavior.

## Working Model

HIR owns high-level record semantics. HIR-to-LIR erases those semantics into
layout. LIR owns only the lowered LLVM struct type name and field layout:

```text
StructNameId -> LirStructDecl -> fields/layout
legacy type_decls -> unchanged printer emission
```

The structured path is a mirror until parity proves it can become authoritative
in a later idea.

## Execution Rules

- Keep every code step behavior-preserving.
- Dual-write structured and legacy data for every struct type declaration this
  plan touches.
- Add mismatch observation before trusting structured rendering.
- Prefer focused proof after each code step and broader/full proof before close.
- If a required field type cannot be represented structurally yet, preserve the
  legacy text and record the gap in `todo.md`.

## Steps

### Step 1: Add Shared Struct Name Table

Goal: define the shared ID/table building block.

Primary targets:
- `src/shared/struct_name_table.hpp`
- include sites that need `StructNameId`

Actions:
- Add `StructNameId`, `kInvalidStructName`, and `StructNameTable`.
- Reuse `TextTable` / `SemanticNameTable`.
- Keep the header narrow and free of LIR/HIR-specific dependencies.

Completion check:
- The project builds with the new shared header included by at least LIR.

### Step 2: Attach Struct Names To LIR Module

Goal: make `LirModule` own a struct-name table sharing the module text table.

Primary targets:
- `src/codegen/lir/ir.hpp`
- LIR module initialization in HIR-to-LIR lowering

Actions:
- Add `StructNameTable struct_names` to `LirModule`.
- Ensure it attaches to the same `TextTable` used by link names.
- Preserve existing `link_name_texts` and `link_names` behavior.

Completion check:
- Existing LIR construction and printing still work with no output changes.

### Step 3: Add Structured Struct Declaration Mirror

Goal: add a minimal `StructNameId -> fields/layout` representation beside
legacy `type_decls`.

Primary targets:
- `src/codegen/lir/ir.hpp`
- verifier support if needed

Actions:
- Add `LirStructField` and `LirStructDecl`.
- Add `std::vector<LirStructDecl> struct_decls`.
- Add `std::unordered_map<StructNameId, size_t> struct_decl_index`.
- Add simple lookup/record helpers if that matches local style.
- Keep `type_decls` unchanged.

Completion check:
- Structured declarations can be stored and looked up without changing printer
  output.

### Step 4: Dual-Write During HIR-To-LIR Type Declaration Generation

Goal: populate the structured mirror whenever legacy `type_decls` are produced.

Primary targets:
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- helper surfaces around `build_type_decls`

Actions:
- Intern each LLVM struct type name into `StructNameTable`.
- Build a matching `LirStructDecl` with field type information that is already
  safely available.
- Keep the existing rendered declaration line in `type_decls`.
- If a field still needs legacy rendered type text, preserve it and mark the
  structured gap explicitly.

Completion check:
- Ordinary structs and template struct/class instantiations produce both
  legacy and structured declarations.

### Step 5: Add Parity / Shadow Rendering

Goal: prove the structured mirror matches legacy declaration text without
switching emission authority.

Primary targets:
- LIR printer-adjacent helpers
- LIR verifier or a local comparison helper
- focused tests if existing coverage does not observe parity

Actions:
- Add a helper that renders `LirStructDecl` to the same LLVM type declaration
  text as legacy `type_decls`.
- Compare structured shadow output against the corresponding legacy line.
- Record/report mismatches in a way suitable for focused tests or debug dumps.
- Keep the printer using legacy `type_decls`.

Completion check:
- Structured shadow rendering matches legacy text for covered declarations.

### Step 6: Focused Validation

Goal: prove the dual-track mirror is behavior-preserving.

Primary targets:
- build/test commands chosen by supervisor for the final packet

Actions:
- Run `cmake --build --preset default`.
- Run focused LIR/codegen coverage for ordinary structs, template struct/class
  instantiations, nested struct fields, global initializers, and
  layout-sensitive codegen.
- Escalate to full CTest if the diff touches shared LIR printer/verifier or
  broad type rendering paths.

Completion check:
- Focused proof passes with unchanged emitted LLVM behavior.
- Any remaining legacy gaps are recorded as follow-up and not hidden.

## Close Criteria

- Shared `StructNameId` / `StructNameTable` exists.
- LIR owns and initializes the table.
- HIR-to-LIR dual-writes structured struct declarations and legacy
  `type_decls`.
- Parity/shadow-render proof exists.
- Legacy rendered emission remains stable.
- Validation passes without expectation downgrades or testcase-shaped
  shortcuts.
