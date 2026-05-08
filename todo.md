Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed Plan Step 6 by making `lookup_abi_struct_layout` return the structured layout on complete owner-key hits and stop after complete owner-key misses instead of consulting rendered compatibility tags.

Extended `frontend_lir_global_type_ref_test` so the focused aggregate fixture proves ABI helper behavior through `classify_aarch64_hfa`: complete owner-key hits use structured layout metadata, complete owner-key misses reject stale rendered compatibility, and incomplete/no-owner metadata still preserves legacy rendered compatibility.

Continued Step 6 by making `lir_owned_type_spec` skip rendered compatibility re-interning whenever a complete aggregate owner key exists but no structured layout is found. Extended `frontend_lir_function_signature_type_ref_test` to prove function return and parameter LIR-owned TypeSpecs preserve the missing owner key instead of being rewritten to stale rendered compatibility, while still dropping AST owner pointers for LIR storage.

## Suggested Next

Next coherent packet: supervisor review/commit for Step 6, or route the next remaining compatibility-retirement slice if broader review finds another active helper family.

## Watchouts

- `lookup_structured_layout` already had the owner-key miss cutoff; this packet aligned the ABI-only helper used by HFA classification and the `lir_owned_type_spec` storage path used for function return/parameter metadata.
- The no-owner compatibility path is still intentionally live for incomplete metadata and is covered by the updated LIR-linked fixture.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^frontend_lir_' --output-on-failure > test_after.log`
