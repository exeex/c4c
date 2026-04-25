Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-Write During HIR-To-LIR Type Declaration Generation

# Current Packet

Implement Step 4 by dual-writing structured LIR struct declarations from the
existing HIR-to-LIR type declaration generation while keeping legacy rendered
`type_decls` as the printer authority.

## Just Finished

Step 4: Dual-Write During HIR-To-LIR Type Declaration Generation is complete.
`build_type_decls()` still appends the same legacy rendered LLVM type
declaration text, and now also records matching `LirStructDecl` mirrors for
ordinary structs, unions, empty structs, padded layouts, base subobject layouts,
and the emitted non-pointer `va_list` struct declaration. Structured
declaration identities are interned from the emitted LLVM struct type spelling
such as `%struct.Name`, not the raw HIR tag.

## Suggested Next

Implement the next packet that proves or consumes the structured declaration
mirror without changing printer authority until the planned printer migration
step.

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
