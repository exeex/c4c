# Current Packet

Status: Active
Source Idea Path: ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe TypeSpec Tag Removal Boundary

## Just Finished

Step 5 - Probe TypeSpec Tag Removal Boundary:
reran the temporary `TypeSpec::tag` deletion probe after the `lvalue.cpp`
member-access migration. The first remaining compile boundary is now
`src/codegen/lir/hir_to_lir/types.cpp`: `lookup_field_chain_layout` still
assigns `ts.tag`, and anonymous/base field-chain recursion still reads
`f.elem_type.tag`. Same-wave residuals also appear in
`src/codegen/llvm/calling_convention.cpp:116-117` ABI legacy fallback and
`src/codegen/lir/verify.cpp:664` direct aggregate signature verification.

## Suggested Next

Next coherent packet: migrate the `types.cpp` field-chain boundary so
`lookup_field_chain_layout` and anonymous/base recursion use structured
`StructNameId`, owner metadata, or an existing named compatibility helper
instead of assigning or reading `TypeSpec::tag`. Keep ABI and verifier
same-wave residuals for later packets unless the field-chain fix needs a tiny
shared helper.

## Watchouts

- The probe edit was temporary: `const char* tag` in
  `src/frontend/parser/ast.hpp` was restored before the post-probe rebuild.
- `src/codegen/lir/hir_to_lir/lvalue.cpp` no longer reads `access.base_ts.tag`;
  remaining `.tag` in that file is `tag_text_id` metadata or `FieldStep::tag`.
- Avoid widening the next packet into ABI or verifier cleanup until the
  field-chain first boundary is cleared.

## Proof

Probe command:
`cmake --build --preset default > test_after.log 2>&1` with `const char* tag`
temporarily disabled failed at the `types.cpp` boundary above.

After reverting the temporary probe edit, `cmake --build --preset default`
passed. The accepted focused baseline remains in `test_before.log`; the current
`test_after.log` is the failed deletion-probe artifact and should be
overwritten by the next executor proof.
