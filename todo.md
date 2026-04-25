Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Parity / Shadow Rendering

# Current Packet

Implement Step 5 by adding parity/shadow rendering for structured
`LirStructDecl` mirrors while keeping legacy rendered `type_decls` as the
printer authority.

## Just Finished

Step 4: Dual-Write During HIR-To-LIR Type Declaration Generation is complete.
`build_type_decls()` still appends the same legacy rendered LLVM type
declaration text, and now also records matching `LirStructDecl` mirrors for
ordinary structs, unions, empty structs, padded layouts, base subobject layouts,
and the emitted non-pointer `va_list` struct declaration. Structured
declaration identities are interned from the emitted LLVM struct type spelling
such as `%struct.Name`, not the raw HIR tag.

## Suggested Next

Add a printer-adjacent helper that renders `LirStructDecl` back to legacy LLVM
type declaration text, compare the shadow-rendered output against the matching
legacy `type_decls` line, and surface mismatches through a focused verifier or
local comparison path without changing emitted LLVM.

## Watchouts

- Do not remove or demote rendered `type_decls`.
- Keep printer output unchanged.
- Keep C++ record semantics out of LIR; this is LLVM struct layout identity
  only.
- Structured fields intentionally mirror the flattened LLVM layout text: base
  subobjects, explicit padding, field payloads, unions, and non-empty empty
  structs are represented as their emitted LLVM field types.
- `StructNameId` values intentionally use LLVM struct type names (`sty` /
  `%struct.__va_list_tag_`) so identity matches declaration and storage type
  spelling.
- `module.struct_decls` remains intentionally inert; `lir_printer.cpp` still
  prints only legacy `type_decls`.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_split_llvm_') > test_after.log 2>&1`.
Proof log: `test_after.log`.
