Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed Plan Step 6 member-access slice by making `member_access_owner_tag_from_type` return structured tags on complete owner-key hits and stop after complete owner-key misses instead of falling back to rendered compatibility tags.

Extended `frontend_lir_call_type_ref_test` with focused LIR member-GEP coverage for structured owner recovery, complete owner-key miss rejection of stale rendered compatibility, and preserved no-owner rendered compatibility.

## Suggested Next

Next coherent packet: supervisor review/commit for Step 6, or route any remaining compatibility-retirement helper only if broader review finds another active LIR owner-tag family.

## Watchouts

- The no-owner compatibility path remains intentionally live for incomplete metadata and is covered through a member-GEP LIR path.
- The complete-miss test injects a compatibility-only HIR layout without indexing it as a structured owner, matching the stale-rendered compatibility shape this slice is retiring.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^frontend_lir_' --output-on-failure > test_after.log`
