Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing Layout Lookup Surfaces

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for HIR-to-LIR struct layout lookup
surfaces. Exact Step 2 helper family: add a shared layout lookup helper under
`src/codegen/lir/hir_to_lir/` near `stmt_emitter_detail::object_align_bytes`,
`llvm_struct_field_slot`, and `llvm_struct_base_slot` that can resolve a
known aggregate `TypeSpec`/rendered type to `StructNameId` and
`LirModule::find_struct_decl(StructNameId)`, then compare with the legacy
`hir::Module::struct_defs` result.

## Suggested Next

Implement Step 2 as a narrow helper-only packet: introduce a shared
`StructNameId`-aware layout lookup helper in the HIR-to-LIR lowering helper
layer, wire it into one low-risk caller such as `object_align_bytes` only when
a `LirModule*`/known aggregate identity is available, and keep existing
`mod.struct_defs.find(ts.tag)` behavior as the fallback and parity baseline.

## Watchouts

Legacy fallback that must remain: every migrated path still needs the rendered
tag lookup through `TypeSpec::tag` and `hir::Module::struct_defs`, including
the current miss behavior (`object_align_bytes` falls back to 8 for unknown
struct/union tags; `sizeof_ts` falls back to 4; field-chain helpers return
false; initializer address/field helpers return nullopt/zeroinitializer paths;
call aggregate sizing can skip zero-sized payloads).

Relevant files/symbols found:

- `src/codegen/lir/ir.hpp`: `LirStructDecl`,
  `LirModule::struct_decls`, `struct_decl_index`,
  `find_struct_decl(StructNameId)`, `record_struct_decl`.
- `src/codegen/lir/types.hpp`: `LirTypeRef::struct_type`,
  `LirTypeRef::union_type`, `has_struct_name_id`,
  `struct_name_id`.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: structured declarations are
  recorded from `mod.struct_def_order` using rendered LLVM type text interned
  in `LirModule::struct_names`; base fields and aggregate fields already carry
  structured `LirTypeRef` mirrors.
- `src/codegen/lir/hir_to_lir/core.cpp`: direct layout lookups in
  `object_align_bytes`, `collect_aarch64_hfa_elements`,
  `classify_aarch64_hfa`, `llvm_struct_base_slot`, and
  `llvm_struct_field_slot(const hir::Module&, ...)`.
- `src/codegen/shared/llvm_helpers.hpp`: `sizeof_ts` is a shared rendered-tag
  size lookup through `mod.struct_defs`; changing it directly is broader than
  the first Step 2 packet unless a HIR-to-LIR wrapper is introduced.
- `src/codegen/lir/hir_to_lir/types.cpp`: field-chain resolution uses
  `StmtEmitter::find_field_chain`,
  `find_field_chain_by_member_symbol_id`, `resolve_field_access`, and
  `resolve_field_access_by_member_symbol_id`, all starting from a rendered
  tag and `mod_.struct_defs`.
- `src/codegen/lir/hir_to_lir/lvalue.cpp`: member access chooses
  `resolved_owner_tag` or `base_ts.tag` before calling the field-chain
  helpers, then emits `LirGepOp` using rendered `FieldStep::tag`.
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`: constant initializer
  paths have local `find_struct_def`, `find_struct_field_index`,
  `find_union_field_index`, `emit_const_struct_fields_impl`, and
  `emit_const_struct_init` surfaces that still consume `mod_.struct_defs`.
- `src/codegen/lir/hir_to_lir/call/args.cpp` and
  `src/codegen/lir/hir_to_lir/call/vaarg*.cpp`: byval/variadic/va_arg
  aggregate handling uses `amd64_fixed_aggregate_byval`,
  `classify_aarch64_hfa`, `object_align_bytes`, direct `mod_.struct_defs`
  payload sizing, and `llvm_cc::amd64_type_size_bytes`.
- `src/codegen/lir/verify.cpp`: existing verification checks structured
  mirrors resolve and match rendered names; it does not yet provide layout
  parity observation for HIR-to-LIR lookup migration.

## Proof

Audit-only packet; no build or ctest required and none run.

Commands run: `git status --short`; read `AGENTS.md`, `plan.md`, and
`todo.md`; `rg` surface searches for `TypeSpec::tag`, `.tag`, `struct_defs`,
`struct_decls`, `StructNameId`, `LirStructDecl`, layout, field, initializer,
GEP, byval/byref, and aggregate call terms; `c4c-clang-tool-ccdb
function-signatures` on `hir_to_lir/core.cpp`, `types.cpp`,
`const_init_emitter.cpp`, and `lvalue.cpp`; focused `nl -ba ... | sed -n`
reads of the relevant helper ranges.
