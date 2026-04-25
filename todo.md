Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Struct Declaration Mirror

# Current Packet

Implement Step 3 by adding a minimal structured LIR struct declaration mirror
beside legacy rendered `type_decls`, without populating it from HIR yet.

## Just Finished

Step 3: Add Structured Struct Declaration Mirror is complete. Added
`LirStructField` and `LirStructDecl` using `StructNameId` / `LirTypeRef`, plus
inert `LirModule::struct_decls` storage, `struct_decl_index`, and simple
record/lookup helpers beside legacy rendered `type_decls`.

## Suggested Next

Implement the next packet that starts populating structured struct declaration
data from HIR while continuing to preserve rendered `type_decls` and printer
output behavior.

## Watchouts

- Do not remove or demote rendered `type_decls`.
- Keep printer output unchanged.
- Keep C++ record semantics out of LIR; this is LLVM struct layout identity
  only.
- `module.struct_decls` is intentionally not consumed by the printer yet.
- `record_struct_decl()` indexes only valid `StructNameId` values; invalid ids
  remain stored but not lookup-addressable.

## Proof

Passed: `cmake --build --preset default > test_after.log 2>&1`.
Proof log: `test_after.log`.
