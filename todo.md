Status: Active
Source Idea Path: ideas/open/106_shared_struct_name_table_for_lir_type_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Attach Struct Names To LIR Module

# Current Packet

Implement Step 2 by making `LirModule` own a `StructNameTable` attached to the
same shared `TextTable` used by link names, without changing emitted output.

## Just Finished

Step 2: Attach Struct Names To LIR Module is complete. Added
`c4c::StructNameTable` ownership to `LirModule` and attached it during
`lower()` to the same shared text table carried by `link_name_texts` /
`link_names`. Legacy `type_decls` storage and printer-facing output behavior
remain unchanged.

## Suggested Next

Implement the next packet that interns LIR struct identities into
`module.struct_names` while continuing to preserve rendered `type_decls`.

## Watchouts

- Do not remove or demote rendered `type_decls`.
- Keep printer output unchanged.
- Keep C++ record semantics out of LIR; this is LLVM struct layout identity
  only.
- `module.struct_names` is attached but intentionally not consumed by the
  printer yet.

## Proof

Passed: `cmake --build --preset default > test_after.log 2>&1`.
Proof log: `test_after.log`.
