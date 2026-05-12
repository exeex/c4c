# Current Packet

Status: Active
Source Idea Path: ideas/open/172_type_identity_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Inventory LIR/BIR Type And Layout Authority

## Just Finished

Step 3 - Inventory LIR/BIR Type And Layout Authority completed as a
code-reading audit.

Files inspected:
- `src/codegen/lir/types.hpp`: `LirTypeRef`, `LirTypeKind`,
  `StructNameId`-backed struct refs, integer bit-width extraction, and
  text-based equality.
- `src/codegen/lir/ir.hpp`: LIR instruction/function/global type carriers,
  `TypeSpec` preservation, structured signature mirrors, `LinkNameId`
  handoffs, `LirStructDecl`, `LirStructuredLayoutObservation`,
  `type_decls`, and extern-decl dedup.
- `src/codegen/lir/hir_to_lir/types.cpp`: HIR-to-LIR field-chain layout
  lookup, `StructuredLayoutLookup`, `StructNameId` propagation, base/anonymous
  aggregate fallback, and `MemberSymbolId` field lookup.
- `src/codegen/lir/lir_printer.cpp`: final LLVM rendering for structured type
  refs, link-name replacement, call targets, and instruction type spellings.
- `src/backend/bir/bir.hpp`: BIR scalar `TypeKind`, ABI metadata structs,
  link/block/slot id carriers, structured type spelling context, aggregate
  call spelling fields, memory-address metadata, and display/compatibility
  names.
- `src/backend/bir/lir_to_bir/lowering.hpp`: lowering context authority
  tables, `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`,
  `BackendStructuredLayoutTable`, `AggregateTypeLayout`, function-local
  route maps, and aggregate parameter metadata.
- `src/backend/bir/lir_to_bir/types.cpp`: structured vs legacy layout table
  construction, type-decl parsing, aggregate layout computation, parity notes,
  scalar lowering, array parsing, and structured-first layout lookup.
- `src/backend/bir/lir_to_bir/call_abi.cpp`: scalar ABI classification,
  structured signature parameter path, signature-text fallback, sret/byval
  metadata, aggregate return lowering, and target-profile ABI decisions.
- `src/backend/bir/lir_to_bir/aggregate.cpp`: byval type normalization,
  aggregate parameter collection, local aggregate slot materialization,
  structured layout selection, leaf-slot traversal, and aggregate copy paths.
- `src/backend/bir/lir_to_bir/module.cpp` and
  `src/backend/bir/lir_to_bir/globals.cpp`: module-level type/layout table
  setup, global/function link-name identity, pointer-initializer resolution,
  global layout lowering, and raw-symbol fallback boundaries.
- `src/backend/bir/bir_printer.cpp` and `src/backend/bir/bir_validate.cpp`:
  BIR dump spelling, structured type rendering, link-name validation, and
  id-aware fallback checks.
- `src/backend/prealloc/legalize.cpp`,
  `src/backend/prealloc/stack_layout/stack_layout.hpp`,
  `src/backend/prealloc/stack_layout/coordinator.cpp`,
  `src/backend/prealloc/stack_layout/analysis.cpp`,
  `src/backend/prealloc/liveness.cpp`, and
  `src/backend/prealloc/regalloc.cpp`: post-BIR type legalization, ABI
  legalization, stack-object layout, prepared value identity, and register
  allocation use of BIR type/size/align metadata.
- `src/backend/mir/aarch64/codegen/emit.cpp`,
  `src/backend/mir/riscv/codegen/emit.cpp`, and live x86 backend/prepared
  files were searched for downstream spelling consumers and target route
  layout decisions.

Authority classifications found:
- LIR structured type/layout authority: `LirTypeRef` carries `LirTypeKind`,
  integer bit width, and optional `StructNameId`. `LirStructDecl` plus
  `LirModule::struct_decls`/`struct_decl_index` is the structured declaration
  mirror for backend layout. `LirFunction::signature_return_type_ref`,
  `signature_param_type_refs`, `signature_params`, and
  `signature_is_variadic` are the structured signature mirrors; `LinkNameId`
  is semantic function/global identity where populated.
- LIR semantic type-string authority: `LirTypeRef::operator==` still compares
  only rendered text, and many LIR operations (`type_str`, `return_type`,
  `args_str`, `LirRet::type_str`, `LirGepOp::element_type`, aggregate
  insert/extract types, global `llvm_type`) carry final LLVM spellings that
  lowering reads as type input. `TypeSpec` remains preserved on older LIR
  structs, function params, stack objects, and globals, but backend-facing
  aggregate layout generally flows through LIR type text plus structured
  mirrors, not through sema canonical type identity.
- LIR ABI/output spelling: `signature_text`, `return_type_str`,
  `llvm_type`, `init_text`, instruction render types, mangled/spec-entry
  strings, and printer replacements are final LLVM/output spelling. They are
  acceptable output surfaces when structured mirrors are present, but become
  compatibility authority when a producer omits the mirror.
- LIR compatibility bridges: `type_decls` is explicitly a legacy textual type
  declaration shadow. `extern_decl_name_map` and raw-name extern dedup are
  fallbacks when no `LinkNameId` exists. HIR-to-LIR field-chain logic still
  recovers nested/base aggregates through rendered tags when `StructNameId`
  metadata is incomplete.
- BIR structured scalar/ABI authority: BIR lowers scalar type identity into
  `bir::TypeKind`; ABI decisions are represented by `CallArgAbiInfo`,
  `CallResultAbiInfo`, `AbiValueClass`, `size_bytes`, `align_bytes`,
  `is_byval`, `is_sret`, and `returned_in_memory`. `legalize.cpp`,
  prealloc stack layout, liveness, and regalloc mostly consume these
  structured fields rather than type strings.
- BIR semantic name authority: `LinkNameId`, `BlockLabelId`, and `SlotNameId`
  are semantic ids when valid. BIR validation rejects mismatched link ids for
  declared functions/globals, and module lowering fails closed when a present
  initializer function id is unknown instead of falling back through raw text.
- BIR structured aggregate layout authority: `BackendStructuredLayoutTable`
  computes layout from `LirStructDecl` entries and `StructNameTable` spelling,
  records structured/legacy parity, and `lookup_backend_aggregate_type_layout`
  prefers structured layout for `%` type names before legacy type-decl parsing.
  Once BIR aggregate params/calls are lowered, downstream code carries
  size/align/ABI flags and scalar slot types.
- BIR semantic type-string authority: aggregate layout lookup is still keyed
  by final type spelling. `BackendStructuredLayoutTable` itself is keyed by
  type-name string, `TypeDeclMap` is string-to-body, and
  `AggregateTypeLayout::element_type_text`/`AggregateField::type_text` recurse
  through text. These strings currently decide aggregate size, alignment,
  byval/sret classification, aggregate slot decomposition, global storage
  size, and initializer lowering when structured layout is absent or when a
  type is inline text.
- BIR display/output spelling: `StructuredTypeSpellingContext`,
  `StructuredTypeDeclSpelling`, `CallInst::structured_return_type_name`,
  `CallInst::return_type_name`, `Value::name`, block labels, slot names,
  call target text, inline asm payloads, and BIR printer output are display or
  final-dump spelling when paired with ids/structured fields.
- Route-local rendering: LIR SSA names, BIR `ValueMap`, local slot names,
  aggregate leaf slot suffixes, `%ret.sret`, temporary copy names, phi/copy
  plans, block lookup strings, and prealloc prepared value names are
  route-local handles. They are not module semantic type identity, but some
  prepared stack-slot recovery parses aggregate leaf-slot spelling suffixes.
- Target-route legacy spelling authority: the active AArch64 LIR route still
  has many direct decisions over `LirTypeRef::str()`/`type_decls`:
  `gen_type_layout`, `gen_resolve_type_decl_body`,
  `gen_is_direct_gp_aggregate_type`, `gen_try_parse_hfa_type`,
  `gen_type_is_sret_aggregate_type`, `gen_type_is_memory_value`, and multiple
  scalar checks such as `type_str == "i32"`. These are semantic ABI/layout
  authority on that route, not mere output spelling.

Highest-risk LIR/BIR/backend gaps:
- Aggregate layout identity is still spelling-keyed at the BIR boundary.
  Structured `LirStructDecl` is preferred, but the lookup key is final
  `%struct...` spelling and field recursion stores type text. Two spellings
  for the same structured type, or one stale `type_decls` shadow, can change
  size/align, byval/sret lowering, and aggregate slot decomposition.
- BIR aggregate ABI classification depends on lowered layout from type text
  before it becomes stable structured metadata. `lower_return_info_from_type`,
  `lower_function_params_with_layouts`, `collect_aggregate_params`, and
  global lowering all use text-normalized aggregate names as decision input.
- The AArch64 backend remains a major spelling-authority island. It parses
  `module.type_decls`, `LirRet::type_str`, `LirCallOp::return_type`,
  `LirCallOp::args_str`/arg text, signature parameter type strings, and
  alloca/load/store type strings to decide ABI class, HFA handling,
  direct-GP aggregate handling, sret, memory-value lowering, stack slot size,
  global initialization, and GEP/struct sizes.
- Structured signature metadata is present but not universal. When
  `signature_return_type_ref` or structured params are missing,
  `signature_text` parsing becomes compatibility authority for return and
  parameter ABI/layout. That is acceptable for hand-built legacy fixtures, but
  unsafe as a generated-LIR producer boundary.
- `LirTypeRef` has structured payload but equality ignores it. A struct type
  with a valid `StructNameId` compares equal to a raw text-only type with the
  same spelling, so ids cannot yet guard all dedup/comparison sites.
- `TypeSpec` still crosses into LIR as preserved C type info for functions,
  globals, stack objects, and older instruction structs. Backend lowering does
  not consistently use sema canonical type identity, so `TypeSpec` fields and
  final LLVM type text can diverge without one authoritative key.
- `type_decls` remains a broad compatibility bridge. BIR uses it as fallback;
  AArch64 uses it directly as layout/ABI authority. Until target routes
  consume structured declarations or BIR-prepared metadata, `type_decls` is
  still a priority spelling-based risk.
- Prepared stack layout and regalloc are mostly structured after BIR, but they
  still inherit earlier aggregate size/align decisions and parse some
  route-local slot-name suffixes for aggregate slices. Bugs in upstream
  aggregate spelling/layout classification become durable prepared metadata.
- Function/global names are mostly id-backed, but raw-name maps remain for
  no-id extern declarations, pointer initializers, and compatibility imports.
  Present ids often fail closed, which is good; missing ids still leave raw
  spelling as semantic bridge.

## Suggested Next

Proceed to Step 4 by building the cross-domain risk map from the Step 1 sema,
Step 2 HIR, and Step 3 LIR/BIR/backend findings. Prioritize gaps where
rendered type spelling still controls equality, layout, ABI class, or route
selection after structured identity exists upstream.

## Watchouts

- Do not classify all backend strings as bugs. Printer output, BIR dumps,
  LLVM type spelling, inline asm text, SSA names, block labels, and local slot
  names are often display/final-output or route-local rendering.
- The priority distinction is whether the string feeds equality, lookup,
  layout, ABI class, storage size, or target route selection. Those are
  semantic authority even when the string is also valid LLVM spelling.
- `BackendStructuredLayoutTable` is a bridge, not a complete identity fix: it
  prefers structured declarations but is still keyed/correlated by final type
  spelling.
- AArch64 direct LIR emission should be treated separately from BIR/prealloc
  preparation. BIR/prealloc is more structured; AArch64 still has extensive
  direct type-string layout authority.
- Follow-up implementation ideas should avoid testcase-shaped fixes such as
  adding one more spelling parser branch. The durable direction is structured
  aggregate/type identity through the backend boundary.

## Proof

- No tests executed; the delegated packet explicitly required code-reading
  audit evidence in `todo.md` and did not require test execution.
- AST-backed inventory used:
  `c4c-clang-tool-ccdb list-symbols` on
  `src/backend/bir/lir_to_bir/types.cpp`,
  `src/backend/bir/lir_to_bir/aggregate.cpp`, and
  `src/backend/bir/lir_to_bir/call_abi.cpp`.
- Direct-read/search evidence used:
  targeted `sed` reads of the files listed above plus `rg` searches for
  `TypeSpec`, `LirTypeRef`, `TypeKind`, `type_decls`, `layout`, `ABI`,
  `byval`, `sret`, `LinkNameId`, `MemberSymbolId`, `gen_type_layout`, and
  target-route type-string consumers.
