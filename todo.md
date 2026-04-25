Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Shared Struct Name Table

# Current Packet

Implement Step 1 by adding the shared `StructNameId` / `StructNameTable`
building block under `src/shared/`, then compile-check the narrow include path.

## Just Finished

Step 1: Add Shared Struct Name Table is complete. Added
`src/shared/struct_name_table.hpp` with `StructNameId`,
`kInvalidStructName`, and `StructNameTable` reusing
`TextTable` / `SemanticNameTable`, then included the shared header from the LIR
package index for later ownership work.

## Suggested Next

Implement Step 2 by adding structured LIR storage that owns/attaches the struct
name table while keeping legacy `type_decls` behavior intact.

## Watchouts

- Do not remove or demote rendered `type_decls`.
- Keep printer output unchanged.
- Keep C++ record semantics out of LIR; this is LLVM struct layout identity
  only.
- `struct_name_table.hpp` is only the shared ID/table building block so far;
  no LIR ownership or printer behavior has changed yet.

## Proof

Passed: `cmake --build --preset default > test_after.log 2>&1`.
Proof log: `test_after.log`.
