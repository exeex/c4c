Status: Active
Source Idea Path: ideas/open/110_lir_struct_type_printer_authority_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Struct Type Surfaces

# Current Packet

## Just Finished

Step 1 - Inventory Struct Type Surfaces completed as a read-only audit.

Audited surface set:

- LIR module declaration authority:
  - `src/codegen/lir/ir.hpp:671`-`679`: `LirModule::type_decls` is still the stored preformatted LLVM declaration vector and `LirModule::struct_decls` / `struct_decl_index` are the structured mirror.
  - `src/codegen/lir/ir.hpp:682`-`709`: `find_struct_decl()` and `record_struct_decl()` are the active structured declaration lookup/write APIs.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1075`-`1077`: lowering attaches the shared text table to `struct_names` and fills `module.type_decls = build_type_decls(hir_mod, &module)`.
- `LirModule::type_decls` producers and consumers:
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:372`-`491`: `build_type_decls()` emits legacy declaration strings and dual-writes `LirStructDecl` fields for va_list, empty/sized structs, unions, bases, padding, and normal fields.
  - `src/codegen/lir/lir_printer.cpp:529`-`530`: `print_llvm()` still prints only `mod.type_decls`; it does not print `mod.struct_decls`.
  - `src/codegen/lir/verify.cpp:409`-`443`: `verify_struct_decl_shadows()` maps legacy `type_decls` by name, renders each structured decl with `render_struct_decl_llvm()`, and requires exact parity.
  - Backend/MIR consumers remain text-based outside the current printer audit scope: `src/backend/bir/lir_to_bir/module.cpp:789` builds a `TypeDeclMap` from `context.lir_module.type_decls`; `src/backend/mir/aarch64/codegen/emit.cpp:1040` and related helpers consume `module.type_decls` for layout/codegen.
- `LirModule::struct_decls` and structured declaration renderer:
  - `src/codegen/lir/ir.hpp:547`-`558`: `LirStructField` and `LirStructDecl` carry `StructNameId`, field `LirTypeRef`s, and packed/opaque flags.
  - `src/codegen/lir/lir_printer.cpp:488`-`516`: `render_struct_decl_llvm()` can render a `LirStructDecl`, but today it is called by verification, not by `print_llvm()` as output authority.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:398`-`488`: structured decl population mirrors the legacy `build_type_decls()` string path, including inherited-base field refs via `lir_aggregate_type_ref()` at lines 450-456 and normal field refs via `lir_field_type_ref()` at lines 474-476.
- `LirTypeRef` / `StructNameId` mirrors:
  - `src/codegen/lir/types.hpp:33`-`208`: `LirTypeRef` stores rendered text, classified kind, optional integer width, and `StructNameId`; equality and streaming still compare/render text only.
  - `src/codegen/lir/types.hpp:54`-`80`: `struct_type()`, `union_type()`, `has_struct_name_id()`, `struct_name_id()`, and `with_struct_name_id()` are the active mirror API.
  - `src/codegen/lir/verify.cpp:52`-`114` and `693`-`703`: verifier checks text/kind consistency and verifies any `StructNameId` mirror resolves in `mod.struct_names` and exactly matches the rendered type text.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:37`-`80`, `src/codegen/lir/hir_to_lir/lvalue.cpp:17`-`23`, `src/codegen/lir/hir_to_lir/call/args.cpp:13`-`23`, and `src/codegen/lir/hir_to_lir/call/target.cpp:17`-`27`: HIR-to-LIR creates struct/union type refs only for exact top-level named aggregate text.
- LIR printer/verifier:
  - `src/codegen/lir/lir_printer.cpp:518`-`530`: `print_llvm()` calls `verify_module()` before output, then emits legacy `type_decls`.
  - `src/codegen/lir/verify.cpp:725`-`730`: module verification runs struct-decl shadow parity, structured layout observations, extern return mirrors, global type mirrors, and function signature mirrors.
  - `src/codegen/lir/verify.cpp:467`-`480`: `verify_global_type_ref_shadows()` requires `LirGlobal::llvm_type_ref` to shadow `llvm_type`.
  - `src/codegen/lir/verify.cpp:575`-`621`: `verify_function_signature_type_ref_shadows()` parses rendered `signature_text` and checks return/param mirror parity.
  - `src/codegen/lir/verify.cpp:227`-`268`: call return and argument mirrors are verified against rendered call text and known struct declarations.
  - `src/codegen/lir/verify.cpp:635`-`665`: structured layout observations are verifier-visible and fail on recorded mismatches.
- Global/function/extern signature and call surfaces:
  - `src/codegen/lir/ir.hpp:529`-`544`: `LirGlobal` keeps rendered `llvm_type` as printer text and optional `llvm_type_ref` as a mirror.
  - `src/codegen/lir/ir.hpp:489`-`493` and `586`-`668`: extern declarations retain `return_type_str` plus `LirTypeRef return_type`; `extern_return_type_ref()` upgrades known struct returns when `struct_decls` has the matching ID.
  - `src/codegen/lir/ir.hpp:498`-`514`: `LirFunction` retains `signature_text` as printer text plus optional return and parameter type refs.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:59`-`68`, `325`, and `343`: globals dual-write `llvm_type_ref` only when the canonical aggregate text exactly matches `llvm_type`; flexible-array literal globals stay text-only.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:95`-`115`, `498`-`577`, `1111`-`1112`, and `1152`: function signatures are still rendered text from `build_fn_signature()`, with `populate_signature_type_refs()` adding mirrors for return and params.
  - `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:585`-`620`: `finalize_module()` converts extern decl maps into `LirExternDecl` and upgrades return mirrors through `extern_return_type_ref()`.
  - `src/codegen/lir/call_args_ops.hpp:13`-`49`, `src/codegen/lir/hir_to_lir/call/args.cpp:246`-`249`, and `src/codegen/lir/hir_to_lir/call/target.cpp:106`-`128`: call ops carry return/arg `LirTypeRef` mirrors while rendered call fields remain text.
- HIR-to-LIR layout lookup and `TypeSpec::tag` surfaces:
  - `src/codegen/lir/hir_to_lir/lowering.hpp:283`-`295`: `StructuredLayoutLookup` is the shared lookup result, carrying legacy decl, structured decl, `StructNameId`, and parity flags.
  - `src/codegen/lir/hir_to_lir/core.cpp:82`-`129`: `lookup_structured_layout()` keys legacy lookup by `TypeSpec::tag`, resolves the structured decl by `llvm_struct_type_str(ts.tag)` through `struct_names`, and records observations.
  - `src/codegen/lir/hir_to_lir/types.cpp:43`-`185`: field-chain lookup still starts from tag strings and `mod_.struct_defs`, but annotates `FieldStep::structured_name_id` when a structured decl exists.
  - `src/codegen/lir/hir_to_lir/lvalue.cpp:31`-`49` and `649`-`669`: member and indexed GEP element type refs use `FieldStep::structured_name_id` / structured lookup when available; text spelling remains the emitted GEP type.
  - `src/codegen/lir/hir_to_lir/core.cpp:447`-`468` and `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:122`-`145`: object alignment routes through `lookup_structured_layout()` but still consumes legacy layout facts.
  - `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:94`-`101`, `516`-`518`, `691`-`709`, `734`-`742`, `767`-`775`, `789`-`820`, and `1355`-`1357`: const/global initializer code partly uses the structured lookup helper, but several member/global GEP paths still directly use `TypeSpec::tag`, `find_struct_def()`, and `mod_.struct_defs`.
  - `src/codegen/lir/hir_to_lir/call/args.cpp:166`-`170` and `src/codegen/lir/hir_to_lir/call/vaarg.cpp:145`-`160`, `264`-`267`: variadic aggregate and `va_arg` paths attempt structured lookup but consume `legacy_decl` for size/layout.
  - `src/codegen/lir/hir_to_lir/core.cpp:402`-`438`: AArch64 HFA classification remains direct `TypeSpec::tag` / `mod.struct_defs` traversal.
- Existing review artifacts read as context:
  - `review/108_step2_global_type_ref_review.md`: records globals as semantic progress with `llvm_type_ref` mirrors while `lir_printer.cpp` remains untouched and legacy global text remains printer authority.
  - `review/109_struct_layout_lookup_route_review.md`: records that layout paths now centralize through `lookup_structured_layout()` but many migrated paths still consume legacy layout facts and need observation/proof before closure.

## Suggested Next

Delegate Step 2 to classify the inventoried surfaces as `printer-authority-ready`,
`legacy-proof-only`, `bridge-required`, `printer-only`, `blocked-by-type-ref`,
or `blocked-by-layout-lookup`.

## Watchouts

- `print_llvm()` still emits `type_decls`, so the structured declaration
  renderer is verifier/shadow infrastructure today, not output authority.
- Several layout paths now observe structured identity but still consume
  legacy `HirStructDef` facts; Step 2 should not overstate these as authority
  switches.
- Existing review artifacts `review/108_step2_global_type_ref_review.md` and
  `review/109_struct_layout_lookup_route_review.md` are untracked in the
  current worktree and were read only.

## Proof

Audit-only packet; no build or ctest was required and no `test_after.log` was
created.

Read-only commands used:

- `sed -n` on `AGENTS.md`, `plan.md`, `todo.md`, the idea 110 source file, and
  relevant review artifacts.
- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`.
- `rg` over `src`, `tests`, and `review` for `type_decls`, `struct_decls`,
  `LirTypeRef`, `StructNameId`, `TypeSpec::tag`, `.tag`, `struct_name`, and
  focused helper/function names.
- `rg --files review` and `rg --files src/codegen/lir/hir_to_lir`.
- `c4c-clang-tool-ccdb list-symbols` for
  `src/codegen/lir/hir_to_lir/lvalue.cpp`; the same compile-db query for
  `src/codegen/lir/hir_to_lir/decls.cpp` failed because that file is not a
  translation unit/path in this build, so targeted `rg`/line reads were used.
- Targeted `nl -ba ... | sed -n` reads of `ir.hpp`, `types.hpp`,
  `lir_printer.cpp`, `verify.cpp`, `hir_to_lir.cpp`, `core.cpp`, `types.cpp`,
  `lvalue.cpp`, `const_init_emitter.cpp`, and HIR-to-LIR call helpers.
