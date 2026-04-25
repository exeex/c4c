Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect LirTypeRef Construction And Struct Type Spelling

# Current Packet

Prepare Step 1 by inspecting `LirTypeRef` construction and struct type spelling
paths before adding the structured mirror.

## Just Finished

Step 1 from `plan.md` is complete as an inspection-only packet.

Concrete `LirTypeRef` shape:
- Definition: `src/codegen/lir/types.hpp`.
- Stored fields today: `text_`, `kind_`, and `integer_bit_width_`.
- Construction surface: default constructor, `const char*`, `std::string` with
  optional `LirTypeKind`, `std::string` plus explicit `LirTypeKind`,
  `std::string` plus explicit `LirTypeKind` and integer width, and
  `LirTypeRef::integer(unsigned)`.
- Classification treats `{...}` and unquoted `%struct.*` as
  `LirTypeKind::Struct`; quoted LLVM struct names like `%"struct.ns::T"` fall
  through to `LirTypeKind::Opaque`.

Struct-valued type-ref construction points found:
- `build_type_decls()` in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` builds
  `LirStructDecl.fields` with `LirTypeRef(...)`. It already has `LirModule*`,
  interns each declaration spelling through `lir_module->struct_names`, and can
  intern field/base struct spellings from `TypeSpec::tag` where those are known.
- `StmtEmitter::emit_member_gep()` in `src/codegen/lir/hir_to_lir/lvalue.cpp`
  creates `LirGepOp.element_type` from `llvm_struct_type_str(step.tag)`. It has
  `FieldStep::tag` and access to `StmtEmitter::module_`, so it can intern the
  same rendered spelling.
- AArch64 and AMD64 va_list lowering emit hard-coded
  `%struct.__va_list_tag_` `LirGepOp.element_type` refs in
  `src/codegen/lir/hir_to_lir/call/vaarg.cpp` and
  `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`. These methods have
  `module_`; `build_type_decls()` already interns the same va_list struct name
  when the target uses a va_list object.
- `hoist_allocas()` in `hir_to_lir.cpp` creates `LirAllocaOp` and `LirStoreOp`
  types from `llvm_alloca_ty(param/local.type.spec)` and `llvm_ty(...)`. It has
  the HIR `Module` and `TypeSpec::tag`, but not `LirModule` or
  `StructNameTable`, so it cannot attach an interned ID without a helper or
  signature change.
- Most statement/expression/call lowering constructs `LirTypeRef` implicitly
  through typed op aggregate initialization from `llvm_ty(...)`,
  `llvm_alloca_ty(...)`, `llvm_field_ty(...)`, `indexed_gep_elem_ty(...)`, or
  hand-written literals. `StmtEmitter` methods generally have both `TypeSpec`
  input and `module_`, so they are viable later factory-call sites; call
  argument strings and signatures remain string-only.

Struct spelling helpers:
- `llvm_struct_type_str(tag)` renders the legacy `%struct.*` or quoted
  `%"struct.*"` spelling and is used directly by type declarations and member
  GEPs.
- `llvm_ty(TypeSpec)`, `llvm_alloca_ty(TypeSpec)`, and
  `llvm_field_ty(HirStructField)` are the main helpers that turn struct/union
  `TypeSpec::tag` values into rendered struct type text.

Minimal Step 2 edit target: add optional `StructNameId` storage and accessors
to `LirTypeRef` in `src/codegen/lir/types.hpp`, preserving all existing
constructors with a default invalid/empty mirror so current construction sites
continue to compile. Do not rewrite HIR-to-LIR call sites in Step 2; centralize
new struct-aware construction helpers in Step 3.

## Suggested Next

Execute Step 2 from `plan.md`: add behavior-preserving optional
`StructNameId` metadata to `LirTypeRef` and only adjust direct compile fallout.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Preserve existing `llvm_ty`, `llvm_alloca_ty`, `TypeSpec::tag`, and rendered
  type string paths.
- Do not expand into globals/functions/extern signature surfaces from idea
  108.
- Do not move layout lookup to `StructNameId`; that belongs to idea 109.
- Major string-only surfaces found: `LirGlobal::llvm_type`, `type_decls`,
  `signature_text`, `LirExternDecl::return_type_str`,
  `LirCallOp::callee_type_suffix`, and `LirCallOp::args_str`.
- `make_lir_call_op()` accepts return and argument type text, but has no
  `StructNameTable` or `StructNameId`; callers may have structured context, the
  helper itself does not.
- Quoted LLVM struct names are currently classified as `LirTypeKind::Opaque`,
  not `Struct`, because `classify()` only recognizes the unquoted `%struct.`
  prefix.

## Proof

Inspection-only packet; no build or CTest run per delegated proof. No
`test_after.log` produced because no implementation files were edited.
