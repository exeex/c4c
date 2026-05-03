# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Codegen Tag Surfaces

## Just Finished

Step 1 - Inventory Codegen Tag Surfaces: classified the `src/codegen`
`TypeSpec::tag` surfaces and identified the first executable migration target.

Inspection commands used:
- `rg -n "\.tag" src/codegen`
- `rg -n "TypeSpec::tag|StructNameId|LirTypeRef|layout|aggregate" src/codegen/shared src/codegen/lir src/codegen/llvm/calling_convention.cpp`
- `sed -n '430,635p' src/codegen/shared/llvm_helpers.hpp`
- `sed -n '1,110p;150,185p;400,420p;565,585p' src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `sed -n '1,190p' src/codegen/lir/hir_to_lir/types.cpp`
- `sed -n '1,50p;590,625p;645,675p' src/codegen/lir/hir_to_lir/lvalue.cpp`
- `sed -n '280,330p;600,685p' src/codegen/lir/hir_to_lir/core.cpp`
- `sed -n '1,35p;120,190p' src/codegen/lir/hir_to_lir/call/args.cpp`
- `sed -n '1,35p' src/codegen/lir/hir_to_lir/call/target.cpp`
- `sed -n '135,280p' src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `sed -n '110,130p;535,550p;730,865p;1395,1410p' src/codegen/lir/hir_to_lir/const_init_emitter.cpp`
- `sed -n '55,80p;180,200p;245,275p' src/codegen/llvm/calling_convention.cpp`
- `sed -n '650,675p' src/codegen/lir/verify.cpp`
- `sed -n '60,75p' src/codegen/lir/hir_to_lir/expr/coordinator.cpp`

Known first deletion-probe boundary:
- `src/codegen/shared/llvm_helpers.hpp:444`, where `llvm_ty(const TypeSpec&)`
  turns struct/union `ts.tag` into final LLVM type spelling with
  `llvm_struct_type_str(ts.tag)`.

Classified codegen surface map:
- `src/codegen/shared/llvm_helpers.hpp:444-445`:
  final LLVM spelling for general `llvm_ty`; also semantic dependency when
  callers use `llvm_ty(TypeSpec)` as the only aggregate identity source.
- `src/codegen/shared/llvm_helpers.hpp:562`:
  final LLVM spelling for `llvm_alloca_ty`.
- `src/codegen/shared/llvm_helpers.hpp:601-602`:
  layout lookup through `mod.struct_defs.find(ts.tag)` in `sizeof_ts`;
  semantic aggregate identity and size/layout fallback.
- `src/codegen/shared/llvm_helpers.hpp:619-620`:
  final LLVM spelling for struct/union field element types.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:39`:
  producer/local scratch ownership of interned `TypeSpec::tag` pointer lifetime
  inside the LIR module.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:43-58,66-87`:
  call/global/signature/field `LirTypeRef` mirror construction; call/ABI type
  ref plus semantic aggregate identity because `StructNameId` is derived from
  rendered `tag` spelling.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:166-173`:
  layout lookup via `lookup_structured_layout` for module object alignment.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:410-411`:
  field/member access and layout lookup for flexible-array global emission.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:576-580`:
  producer/local scratch for base-class `TypeSpec`, then semantic aggregate
  identity for `LirTypeRef` construction from `base.tag`.
- `src/codegen/lir/hir_to_lir/types.cpp:47,56-63,81-86,104,124-131,148-156,175`:
  field/member access and layout lookup. `tag` is used as the legacy
  `struct_defs` key, recursive anonymous-member/base traversal key, and
  `FieldStep` rendered spelling; `structured_name_id` is already available
  beside it.
- `src/codegen/lir/hir_to_lir/lvalue.cpp:35-37`:
  final LLVM spelling for GEP aggregate type text, with `structured_name_id`
  already used as the semantic mirror where present.
- `src/codegen/lir/hir_to_lir/lvalue.cpp:607-618`:
  field/member access. Preferred owner tag comes from resolved HIR member
  metadata; fallback reads `base_ts.tag` and then resolves by legacy tag key.
- `src/codegen/lir/hir_to_lir/lvalue.cpp:649-669`:
  indexed GEP aggregate element type; layout lookup and LIR aggregate type ref.
- `src/codegen/lir/hir_to_lir/core.cpp:285`:
  diagnostic/verifier observation text for structured layout, not semantic
  authority by itself.
- `src/codegen/lir/hir_to_lir/core.cpp:310-325`:
  central layout lookup bridge. Uses `ts.tag` for legacy `struct_defs` lookup
  and derives rendered text for `StructNameId` lookup; this is the strongest
  shared semantic aggregate identity migration target.
- `src/codegen/lir/hir_to_lir/core.cpp:612-632`:
  AArch64 HFA and aggregate layout classification; layout/ABI type ref through
  legacy `struct_defs` keyed by `tag`.
- `src/codegen/lir/hir_to_lir/core.cpp:675-681`:
  object alignment route; layout lookup using `lookup_structured_layout`.
- `src/codegen/lir/hir_to_lir/expr/coordinator.cpp:68-69`:
  field/member access for member function-pointer signature discovery through
  legacy `struct_defs` lookup.
- `src/codegen/lir/hir_to_lir/call/args.cpp:16-22`:
  call argument `LirTypeRef` mirror; call/ABI type ref and semantic identity
  because the mirror `StructNameId` is interned from rendered text.
- `src/codegen/lir/hir_to_lir/call/args.cpp:127-188`:
  variadic aggregate detection and payload sizing; call/ABI type ref plus
  layout lookup through `lookup_structured_layout`.
- `src/codegen/lir/hir_to_lir/call/target.cpp:20-26`:
  call target return/parameter `LirTypeRef` mirror; call/ABI type ref and
  semantic identity via rendered-text `StructNameId` interning.
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp:147-155,270-272`:
  `va_arg` aggregate detection and payload sizing; call/ABI type ref plus
  layout lookup through `lookup_structured_layout`.
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:121-122`:
  const-init aggregate layout lookup; semantic aggregate identity through
  legacy `struct_defs` key after structured-layout observation.
- `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:544-545,736-860,1403`:
  const-init field/member access and final constant GEP spelling. Uses legacy
  tag to find struct fields, compute offsets, and render aggregate GEP types.
- `src/codegen/llvm/calling_convention.cpp:71-72,189,252`:
  ABI classification layout lookup for SysV AMD64 aggregate classification,
  size, and byval decisions.
- `src/codegen/lir/verify.cpp:664`:
  compatibility/verifier expectation for direct aggregate signature mirrors;
  it checks old `TypeSpec` text against `StructNameId` mirrors and should be
  preserved until migrated with the carrier contract.

## Suggested Next

First executable migration target for Step 2/3: extend the central
`stmt_emitter_detail::lookup_structured_layout` contract in
`src/codegen/lir/hir_to_lir/core.cpp` and its declaration in `lowering.hpp` so
callers can supply or recover `StructNameId`/structured aggregate identity
without deriving it from `TypeSpec::tag` rendered text. Keep the legacy
`struct_defs` fallback named as compatibility, then migrate one narrow caller
family, preferably the field-chain route in
`src/codegen/lir/hir_to_lir/types.cpp`, because it already carries
`structured_name_id` into `FieldStep` and feeds `lvalue.cpp` GEP generation.

## Watchouts

- `llvm_helpers.hpp:444` is the current first deletion-probe boundary, but the
  helper itself is also a final LLVM spelling boundary. Avoid "fixing" it by
  deleting spelling; migrate semantic callers to structured carriers first.
- `lookup_structured_layout` is the central bridge where legacy
  `mod.struct_defs.find(ts.tag)` and structured `StructNameId` lookup meet.
  A narrow API extension there should reduce many layout, field-chain,
  `va_arg`, and const-init residuals without inventing a renamed rendered
  semantic field.
- The existing carriers to prefer are `StructNameId`, `LirTypeRef`,
  `LirModule::struct_names`, `LirModule::find_struct_decl`, and
  `StructuredLayoutLookup::structured_name_id`.
- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve final LLVM spelling, diagnostics, dumps, mangling,
  ABI/link-visible text, and explicitly named compatibility payloads.
- Treat parser/Sema/HIR residuals as parent-idea scope only if a codegen route
  proves an upstream producer carrier is still missing.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Use deletion builds only as probes; do not commit a broken deletion build.

## Proof

Inventory-only packet; no build required and no `test_after.log` was produced
by the delegated proof contract.
