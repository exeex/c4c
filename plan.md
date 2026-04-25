# LIR StructNameId For Globals Functions And Externs

Status: Active
Source Idea: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md

## Purpose

Extend the `StructNameId`-aware `LirTypeRef` mirror from idea 107 into the
major LIR signature and declaration surfaces while preserving all currently
rendered LLVM text.

Goal: globals, functions, call sites, and extern declarations carry structured
struct type identity beside their legacy printer strings.

Core Rule: dual-write structured type refs only where the type identity is
already known; do not make structured type refs the printer authority in this
plan.

## Read First

- `ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md`
- `ideas/closed/107_lir_struct_name_id_type_ref_mirror.md`
- `src/codegen/lir/types.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/lir_printer.cpp`

## Current Targets

- `LirGlobal::llvm_type` and global lowering of struct-bearing global types.
- `LirFunction::signature_text` and function definition/declaration lowering
  for return and parameter type mirrors.
- `LirExternDecl::return_type_str` and extern declaration dedup/output.
- `LirCallOp::return_type` and call argument type construction where struct
  identity is available.
- Helper-generated function type suffixes and signature fragments that still
  only preserve rendered type text.
- LIR verifier checks that can observe structured-vs-rendered parity without
  changing output.

## Non-Goals

- Do not switch `type_decls`, `signature_text`, `llvm_type`, or
  `return_type_str` printer authority to structured rendering.
- Do not remove legacy text fields.
- Do not change emitted LLVM output.
- Do not alter aggregate layout, ABI classification, or call lowering
  semantics.
- Do not use `StructNameId` for function/global symbol identity; keep
  `LinkNameId` authoritative for symbols.

## Working Model

- `LirTypeRef::struct_type(rendered_text, StructNameId)` is the structured
  mirror for a rendered LLVM struct type.
- Legacy strings remain compatibility and printer text.
- Structured mirrors are accepted only when they shadow-render to exactly the
  same type text already used by the printer.
- Verifier coverage should catch missing or mismatched struct IDs on surfaces
  that claim to carry a structured mirror.

## Execution Rules

- Keep each step behavior-preserving and testable.
- Prefer helper extraction for converting known HIR `TypeSpec` struct types to
  `LirTypeRef` with `StructNameId`; avoid ad hoc named-case matching.
- When a field already uses `LirTypeRef`, enrich it rather than adding a
  parallel field.
- When a surface still requires a legacy string for printing, keep that string
  and add the smallest structured mirror needed for parity checking.
- Add verifier assertions after the mirror data exists; do not make verifier
  expectations outrun lowering.
- Use focused tests that compare current output and inspect structured mirrors.
  Escalate to broader validation after multiple signature surfaces have landed.

## Ordered Steps

### Step 1: Audit Existing Signature And Type-Ref Surfaces

Goal: identify which target surfaces already carry `LirTypeRef` and which still
need a structured mirror.

Primary targets:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/types.cpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/hir_to_lir/call/`
- `src/codegen/lir/verify.cpp`

Actions:

- Inspect `LirGlobal`, `LirFunction`, `LirExternDecl`, and `LirCallOp`.
- Trace where global, function, extern, and call type strings are built.
- Locate any helper that can map HIR struct types to `StructNameId`.
- Record target files and the first executable packet in `todo.md`; do not edit
  the source idea for routine audit findings.

Completion check:

- The executor can name the exact code fields and builders that must be updated
  before starting implementation.

### Step 2: Dual-Write Global Type Mirrors

Goal: make struct-bearing globals carry a structured `LirTypeRef` mirror beside
their rendered `llvm_type`.

Primary targets:

- Global lowering in `src/codegen/lir/hir_to_lir/`
- `LirGlobal` in `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- Focused LIR/global tests under `tests/`

Actions:

- Add or enrich the global type mirror using `StructNameId` where the lowered
  HIR type already identifies a struct.
- Preserve `llvm_type` as the printer text.
- Add verifier parity checks between the mirror text and `llvm_type` when the
  mirror is present.
- Cover struct global declarations and definitions, including extern globals if
  represented on this surface.

Completion check:

- Existing global output is unchanged, and focused tests prove struct global
  mirrors carry the expected `StructNameId`.

### Step 3: Dual-Write Function Signature Mirrors

Goal: carry structured type refs for function return and parameter types without
replacing `signature_text`.

Primary targets:

- Function lowering in `src/codegen/lir/hir_to_lir/`
- `LirFunction` in `src/codegen/lir/ir.hpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/verify.cpp`

Actions:

- Add structured return and parameter type mirrors, or enrich existing function
  type metadata if an equivalent field already exists.
- Preserve `signature_text` exactly for printer output.
- Keep `LinkNameId` as the function symbol identity.
- Verify mirror text parity for struct-bearing returns and parameters.
- Cover definitions and declarations, including template-instantiated
  signatures where applicable.

Completion check:

- Function signature LLVM text is unchanged, and tests observe
  `StructNameId`-aware mirrors for struct return and parameter types.

### Step 4: Dual-Write Extern Declaration Mirrors

Goal: make extern declarations carry structured return type identity while
keeping `return_type_str` as compatibility text.

Primary targets:

- Extern declaration lowering and dedup paths in `src/codegen/lir/hir_to_lir/`
- `LirExternDecl` in `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/lir_printer.cpp`

Actions:

- Enrich `LirExternDecl::return_type` with `StructNameId` when an extern
  returns a known struct type.
- Preserve `return_type_str` and declaration output.
- Ensure dedup logic does not collapse distinct structured identities based
  only on rendered text when that would hide a mismatch.
- Add verifier parity for `return_type` against `return_type_str`.

Completion check:

- Extern declaration output remains unchanged, and struct-return externs carry
  structured return type mirrors.

### Step 5: Dual-Write Call Return And Argument Type Mirrors

Goal: ensure call sites preserve structured type identity for struct-bearing
return and argument type refs where that identity is already known.

Primary targets:

- `src/codegen/lir/hir_to_lir/call/`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `LirCallOp` in `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`

Actions:

- Enrich call return `LirTypeRef` construction with `StructNameId`.
- Preserve or add structured argument type refs where call argument formatting
  currently only stores rendered typed-call text.
- Keep formatted call-site text unchanged.
- Add verifier or focused test coverage for struct-bearing direct and indirect
  calls as supported by the existing model.

Completion check:

- Call output is unchanged, and struct return/argument type mirrors survive
  call lowering and parsing/formatting helpers.

### Step 6: Validate Parity And Broader Stability

Goal: prove the dual-track mirrors did not change behavior and are ready for
the next layout-lookup idea.

Primary targets:

- Focused struct/template/global-init/backend LIR tests.
- Repo-native build and CTest subsets chosen by the supervisor.

Actions:

- Run a fresh build or compile proof for each implementation packet.
- Run focused tests for globals, function signatures, extern declarations, and
  calls after their respective steps.
- After the signature surfaces are complete, run a broader LIR/backend subset
  or full suite as directed by the supervisor.
- Leave remaining printer-authority cleanup for idea 110.

Completion check:

- Acceptance criteria from the source idea are satisfied: target surfaces carry
  struct-aware type refs, rendered output is unchanged, parity checks cover the
  new mirrors, and legacy text fields remain in place.
