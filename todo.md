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

Step 5: Add Parity / Shadow Rendering is complete. Added
`render_struct_decl_llvm()` as a printer-adjacent helper that renders a
`LirStructDecl` mirror to the same LLVM type declaration text as legacy
`type_decls`, including empty, normal, packed, and opaque structured forms.
`verify_module()` now shadow-compares each structured declaration against the
matching legacy `type_decls` line by struct name and reports missing or
mismatched shadows through `LirVerifyError`. `print_llvm()` still emits only
`mod.type_decls`.

## Suggested Next

Run Step 6 focused validation for the dual-track mirror, using the supervisor's
chosen broader subset to prove behavior-preserving output across representative
struct/codegen coverage.

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
- `module.struct_decls` remains shadow-only; `lir_printer.cpp` still prints
  only legacy `type_decls`.
- `verify_module()` now depends on the printer-adjacent shadow helper, so any
  future structured declaration form must stay text-equivalent to the legacy
  declaration line before emission authority changes.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_split_llvm_|frontend_hir_lookup_tests$)') > test_after.log 2>&1`.
Proof log: `test_after.log`.
