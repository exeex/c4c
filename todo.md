Status: Active
Source Idea Path: ideas/open/110_lir_struct_type_printer_authority_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Readiness And Blockers

# Current Packet

## Just Finished

Step 2 - Classify Readiness And Blockers completed as a read-only audit for
idea 110. Classification is by source-idea label:

- `printer-authority-ready` switch candidates:
  - `LirModule::struct_decls` plus `render_struct_decl_llvm()` for LLVM struct
    type declarations. `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:372`-`491`
    dual-writes `LirStructDecl` data for va_list, empty/sized structs, unions,
    bases, padding, and normal fields; `src/codegen/lir/lir_printer.cpp:488`-`516`
    can render the structured declaration; `src/codegen/lir/verify.cpp:409`-`443`
    requires exact parity with each legacy `type_decls` line before printing.
    This is the only audited surface that looks like a near-term printer
    authority switch candidate, subject to Step 3 emitted-LLVM proof.
  - Member/indexed GEP element type refs for named aggregates are local
    type-ref authority candidates, not declaration-output authority candidates:
    `src/codegen/lir/hir_to_lir/types.cpp:43`-`185` records
    `FieldStep::structured_name_id`, and
    `src/codegen/lir/hir_to_lir/lvalue.cpp:17`-`23`, `31`-`49`, and `649`-`669`
    use it when constructing `LirGepOp` type refs.

- `legacy-proof-only` surfaces:
  - `LirModule::type_decls` as the shadow declaration text for LIR declaration
    parity. `src/codegen/lir/ir.hpp:671`-`679` still stores the legacy vector,
    and `src/codegen/lir/lir_printer.cpp:529`-`530` still prints it today, but
    once declaration authority switches this vector should remain only as a
    mismatch-observation bridge until downstream consumers are migrated.
  - `verify_struct_decl_shadows()` and `render_struct_decl_llvm()` as verifier
    infrastructure are `legacy-proof-only` after the switch: they should keep
    proving structured-vs-legacy parity, not remain the source of final output.
  - Global/function/extern/call mirror verifiers are also proof-only for their
    mirrored text fields: `verify_global_type_ref_shadows()` at
    `src/codegen/lir/verify.cpp:467`-`480`,
    `verify_extern_decl_shadows()` at `src/codegen/lir/verify.cpp:445`-`465`,
    `verify_function_signature_type_ref_shadows()` at
    `src/codegen/lir/verify.cpp:575`-`621`, and call checks at
    `src/codegen/lir/verify.cpp:227`-`268`.
  - `StructuredLayoutLookup` parity recording is proof-only for layout
    readiness: `src/codegen/lir/hir_to_lir/core.cpp:82`-`135` records
    observations and `src/codegen/lir/verify.cpp:635`-`670` fails mismatches,
    but layout facts still come from legacy HIR declarations.

- `printer-only` final text surfaces:
  - `LirFunction::signature_text` remains the signature printer source:
    `src/codegen/lir/ir.hpp:498`-`514`,
    `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:498`-`577`,
    `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1111`-`1112` and `1151`-`1152`,
    and `src/codegen/lir/lir_printer.cpp:464`-`468`. Tests
    `frontend_lir_function_signature_type_ref_test` assert the printer still
    uses `signature_text`.
  - `LirGlobal::llvm_type` remains global printer text:
    `src/codegen/lir/ir.hpp:529`-`544` and
    `src/codegen/lir/lir_printer.cpp:543`-`550`. Test
    `frontend_lir_global_type_ref_test` asserts the printer still uses legacy
    `llvm_type` text.
  - `LirExternDecl::return_type_str` remains extern declaration printer text:
    `src/codegen/lir/ir.hpp:489`-`493`, `586`-`668`,
    `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:585`-`620`, and
    `src/codegen/lir/lir_printer.cpp:574`-`579`. Test
    `frontend_lir_extern_decl_type_ref_test` asserts the printer still uses
    `return_type_str`.
  - Rendered call-site instruction text remains final output only even when
    return/arg mirrors exist. Tests `frontend_lir_call_type_ref_test` assert
    formatted call-site text remains the printer source.

- `bridge-required` surfaces:
  - Backend and MIR consumers still require legacy declaration text:
    `src/backend/bir/lir_to_bir/module.cpp:789` builds a `TypeDeclMap` from
    `context.lir_module.type_decls`, and
    `src/backend/mir/aarch64/codegen/emit.cpp:1033`-`1048` parses
    `module.type_decls` for struct layout. Backend tests such as
    `tests/backend/backend_lir_to_bir_notes_test.cpp:1026`, `1188`, `1224`,
    `1254`, `1283`, `1501`, `1534`, `1567`, `1733`, `1798`, and `1871`
    hand-build only `type_decls`, so demoting the vector without a bridge would
    break backend coverage.
  - Function signatures need a structured signature bridge before their text
    can be demoted. The current mirrors cover return/param type fragments, but
    the full printed signature still includes linkage, visibility, byval align,
    variadic markers, template comments, parameter names, and function symbol
    spelling in `build_fn_signature()` and `render_fn()`.
  - Global and extern declarations need broader structured declaration models
    before `llvm_type` / `return_type_str` can be demoted. Current mirrors prove
    exact top-level aggregate type identity only; they do not own initializer
    text, linkage/visibility, alignment, or declaration spelling.

- `blocked-by-type-ref` surfaces:
  - `LirTypeRef` is still a text-first mirror wrapper:
    `src/codegen/lir/types.hpp:33`-`208` stores rendered text, compares and
    streams text, and carries `StructNameId` only as an optional mirror.
  - HIR-to-LIR only attaches `StructNameId` for exact top-level named aggregate
    text. Helpers in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:37`-`80`,
    `src/codegen/lir/hir_to_lir/lvalue.cpp:17`-`23`,
    `src/codegen/lir/hir_to_lir/call/args.cpp:13`-`23`, and
    `src/codegen/lir/hir_to_lir/call/target.cpp:17`-`27` fall back to text for
    pointers, arrays, aggregate literals, byval wrappers, and non-exact text.
  - Tests make that boundary explicit:
    `frontend_lir_function_signature_type_ref_test` expects non-aggregate
    mirrors to lack `StructNameId`; `frontend_lir_global_type_ref_test` leaves
    flexible-array literal globals text-only; `frontend_lir_call_type_ref_test`
    verifies known struct call mirrors but still expects formatted call text.

- `blocked-by-layout-lookup` surfaces:
  - `lookup_structured_layout()` resolves a `StructNameId` and records parity,
    but still keys legacy lookup by `TypeSpec::tag` and returns
    `legacy_decl` as the layout fact source:
    `src/codegen/lir/hir_to_lir/lowering.hpp:283`-`295` and
    `src/codegen/lir/hir_to_lir/core.cpp:106`-`135`.
  - Object alignment, variadic aggregate arguments, and `va_arg` attempt
    structured lookup but consume legacy layout facts:
    `src/codegen/lir/hir_to_lir/core.cpp:447`-`468`,
    `src/codegen/lir/hir_to_lir/call/args.cpp:166`-`170`, and
    `src/codegen/lir/hir_to_lir/call/vaarg.cpp:145`-`160`, `264`-`267`.
  - Const/global initializer paths are still mixed. The shared helper exists at
    `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:94`-`101` and is used
    for nested initializer traversal at `1355`-`1357`, but global/member GEP
    shortcuts still use `TypeSpec::tag`, `find_struct_def()`, and
    `mod_.struct_defs` directly at `516`-`518`, `691`-`709`, `734`-`742`,
    `767`-`775`, and `789`-`820`.
  - AArch64 HFA classification remains direct `TypeSpec::tag` /
    `mod.struct_defs` traversal in `src/codegen/lir/hir_to_lir/core.cpp:402`-`438`.

Summary: the struct declaration renderer/`struct_decls` pair is the only
printer-authority switch candidate. `type_decls` can become proof-only for LIR
printing only after backend/MIR bridges exist or continue receiving legacy
text. Signature/global/extern/call strings remain printer-only or
bridge-required, while broader type-ref and layout-lookup coverage block any
larger demotion of rendered type strings.

## Suggested Next

Delegate Step 3 to run the supervisor-selected focused proof for struct
declarations, signatures, globals, externs, calls, and global-init/layout
coverage, then record whether emitted LLVM confirms the Step 2 classification.

## Watchouts

- Do not classify signature/global/extern/call type refs as printer authority
  switches. Current tests explicitly assert those printers still use rendered
  text fields.
- A declaration-output switch from `type_decls` to `struct_decls` is plausible
  for LIR printing, but backend and MIR consumers still parse `type_decls`.
  That makes downstream compatibility a bridge requirement, not a reason to
  switch every consumer in the audit idea.
- Layout parity is now verifier-visible, but many paths still consume
  `legacy_decl` or direct `mod_.struct_defs` facts. Treat those as
  `blocked-by-layout-lookup` until a later implementation owns structured
  layout facts, not just structured observation.
- Existing review artifacts `review/108_step2_global_type_ref_review.md` and
  `review/109_struct_layout_lookup_route_review.md` are untracked in the
  current worktree and were read only.

## Proof

Audit-only packet; no build or ctest was required and no `test_after.log` was
created.

Read-only commands used:

- `git status --short`.
- `sed -n` on `AGENTS.md`, `plan.md`, `todo.md`, idea 110, and review
  artifacts for ideas 108 and 109.
- `rg -n` over `src` and `tests` for `type_decls`, `struct_decls`,
  `render_struct_decl_llvm`, verifier shadow helpers, `StructuredLayoutLookup`,
  `lookup_structured_layout`, `TypeSpec::tag`, `LirTypeRef`, `StructNameId`,
  signature/global/extern/call mirror fields, and focused frontend/backend
  tests.
- Targeted `nl -ba ... | sed -n` reads of `src/codegen/lir/ir.hpp`,
  `src/codegen/lir/lir_printer.cpp`, `src/codegen/lir/verify.cpp`,
  `src/codegen/lir/types.hpp`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`,
  `src/codegen/lir/hir_to_lir/core.cpp`,
  `src/codegen/lir/hir_to_lir/lowering.hpp`,
  `src/codegen/lir/hir_to_lir/types.cpp`,
  `src/codegen/lir/hir_to_lir/lvalue.cpp`,
  `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`,
  `src/codegen/lir/hir_to_lir/call/args.cpp`,
  `src/codegen/lir/hir_to_lir/call/target.cpp`,
  `src/codegen/lir/hir_to_lir/call/vaarg.cpp`,
  `src/backend/bir/lir_to_bir/module.cpp`, and
  `src/backend/mir/aarch64/codegen/emit.cpp`.
