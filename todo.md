Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Struct-Aware TypeRef Factories

# Current Packet

Execute Step 3 from `plan.md`: add narrow struct-aware `LirTypeRef`
construction helpers that dual-write existing rendered type text plus
`StructNameId`, without changing lowering call sites yet.

## Just Finished

Step 3 from `plan.md` is complete.

`LirTypeRef` now has explicit `struct_type(rendered_text, StructNameId)` and
`union_type(rendered_text, StructNameId)` factories in
`src/codegen/lir/types.hpp`. Both preserve the exact caller-provided rendered
LLVM type string, classify the ref as `LirTypeKind::Struct`, and attach the
known `StructNameId` mirror through the existing accessor path.

Existing fallback constructors and string-authoritative behavior remain
unchanged. No HIR-to-LIR lowering, verifier, printer, or test call sites were
changed in this packet.

## Suggested Next

Execute Step 4 from `plan.md`: thread `StructNameId` through HIR-to-LIR
`LirTypeRef` creation only where structured identity is already available and
the legacy rendered spelling can be preserved.

Intended Step 4 call-site targets are struct or union typed refs in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, especially paths that already
intern names through the module `StructNameTable` while producing
`%struct.*`-style rendered type text. Keep string-only signature/global/extern
surfaces out of this step unless needed to keep a touched `LirTypeRef`
construction coherent.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Preserve existing `llvm_ty`, `llvm_alloca_ty`, `TypeSpec::tag`, and rendered
  type string paths.
- Do not add positional `LirTypeRef(..., StructNameId)` constructor overloads
  in the third argument slot; they are ambiguous with the unsigned
  integer-width constructor.
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
  not `Struct`, through fallback constructors because `classify()` only
  recognizes the unquoted `%struct.` prefix. The new explicit factories classify
  known struct and union refs as `Struct` without changing the rendered text.

## Proof

`cmake --build --preset default > test_after.log 2>&1` passed.
`test_after.log` is the proof log.
