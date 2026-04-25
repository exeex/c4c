Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Thread StructNameId Through HIR-To-LIR TypeRef Creation

# Current Packet

Execute Step 4 from `plan.md`: thread `StructNameId` through HIR-to-LIR
`LirTypeRef` creation only where structured identity is already available and
the legacy rendered spelling can be preserved.

## Just Finished

Step 4 from `plan.md` is complete for the first HIR-to-LIR `LirTypeRef`
creation paths with reliable struct identity.

`build_type_decls()` now attaches `StructNameId` mirrors to direct
struct/union field refs and base-subobject refs in structured declaration
mirrors while preserving the exact `llvm_field_ty()` and `llvm_ty()` rendered
text. Padding, byte-array union storage, anonymous/invalid tags, pointer fields,
and array-shaped field refs continue through legacy string-only
`LirTypeRef` construction.

Member-access GEP element types now wrap resolved `FieldStep` struct/union tags
with the Step 3 factories when `module_` is available. AMD64 and AArch64
non-pointer `va_list` GEP element types now attach the `%struct.__va_list_tag_`
mirror through a local accessor. All paths keep the same rendered LLVM type
strings and retain fallback string-only behavior when no `LirModule` is
available.

## Suggested Next

Have the supervisor decide whether Step 4 needs a follow-up packet for
additional typed lowering surfaces outside the owned files, or move to the next
plan step if these representative HIR-to-LIR mirrors are sufficient.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Preserve existing `llvm_ty`, `llvm_alloca_ty`, `TypeSpec::tag`, and rendered
  type string paths.
- Do not expand into globals/functions/extern signature surfaces from idea
  108.
- Do not move layout lookup to `StructNameId`; that belongs to idea 109.
- Deferred Step 4 surfaces: array-shaped struct fields such as
  `[N x %struct.X]`, pointer fields rendered as `ptr`, globals/functions/extern
  signature text, and other non-owned GEP sites that only have rendered type
  text in this packet.
- `make_lir_call_op()` still accepts return and argument type text and has no
  `StructNameTable` or `StructNameId`; callers may have structured context, but
  the helper itself does not.

## Proof

`cmake --build --preset default > test_after.log 2>&1` passed.
Additional supervisor regression guard passed with matching before/after
`ctest --test-dir build -j --output-on-failure -R
'^(positive_sema_|abi_|positive_split_llvm_)'` logs and
`--allow-non-decreasing-passed`: 38 passed before, 38 passed after, 0 failed.
