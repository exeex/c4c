Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed Plan Step 6 LIR aggregate helper slice by making `lir_aggregate_structured_name_id` try structured owner-key identity before rendered compatibility/final-spelling paths and stop after complete owner-key misses instead of returning compatibility name ids.

Added focused LIR-linked coverage for `lir_aggregate_structured_name_id` through public lowering paths: global type refs prove complete owner-key hits produce verifier-consistent structured owner text/name-id mirrors instead of stale rendered text, and complete misses do not return or intern stale compatibility name ids; signature type refs prove incomplete/no-owner metadata still preserves the compatibility name-id path.

## Suggested Next

Next coherent packet: supervisor review/commit for Step 6, or route any remaining compatibility-retirement helper only if broader review finds another active LIR owner-name family.

## Watchouts

- The no-owner compatibility path remains intentionally live for incomplete metadata.
- Owner-key hits normalize aggregate-name mirrors to the structured owner tag; ABI fragments such as `ptr byval(%struct.X)` remain raw mirrors.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^frontend_lir_' --output-on-failure > test_after.log`
