Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Shared Struct Name Table

# Current Packet

Implement Step 1 by adding the shared `StructNameId` / `StructNameTable`
building block under `src/shared/`, then compile-check the narrow include path.

## Just Finished

Activated the runbook from idea 106 after updating the idea to require
dual-track structured/legacy operation, parity proof, and no legacy demotion in
the first slice.

## Suggested Next

Add `src/shared/struct_name_table.hpp` with `StructNameId`,
`kInvalidStructName`, and `StructNameTable`, then include it from the LIR model
surface that will own the table in Step 2.

## Watchouts

- Do not remove or demote rendered `type_decls`.
- Keep printer output unchanged.
- Keep C++ record semantics out of LIR; this is LLVM struct layout identity
  only.
- Preserve the existing shared `TextTable` / `SemanticNameTable` pattern.

## Proof

Not run yet. Step 1 proof should start with `cmake --build --preset default`
or a narrower compile check if the repo has one available for shared/LIR
headers.
