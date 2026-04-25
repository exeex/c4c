Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Optional StructNameId Metadata To LirTypeRef

# Current Packet

Execute Step 2 from `plan.md`: add behavior-preserving optional
`StructNameId` metadata to `LirTypeRef` without changing rendered type-string
authority.

## Just Finished

Step 2 from `plan.md` is complete.

`LirTypeRef` now carries a default-invalid `StructNameId` mirror in
`src/codegen/lir/types.hpp`, exposed through `struct_name_id()`,
`has_struct_name_id()`, `set_struct_name_id()`, and
`with_struct_name_id()`.

Existing constructors, `integer_bit_width()` behavior, string conversion,
comparison, and stream rendering remain string-authoritative and compatible
with current call sites. Direct compile fallout found and fixed: positional
constructor overloads using `StructNameId` conflicted with the existing unsigned
integer-width constructor because `StructNameId` is a `uint32_t`; the final API
uses explicit accessors instead of ambiguous constructor slots.

## Suggested Next

Execute Step 3 from `plan.md`: add narrow struct-aware construction helpers at
the lowering boundary so known struct type strings can attach the mirrored
`StructNameId` without moving printer/verifier authority away from rendered
type text.

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
  not `Struct`, because `classify()` only recognizes the unquoted `%struct.`
  prefix.

## Proof

`cmake --build --preset default > test_after.log 2>&1` passed.
`test_after.log` is the proof log.
