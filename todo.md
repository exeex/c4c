Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed a partial Plan Step 6 LIR aggregate helper slice by making `lir_aggregate_structured_name_id` try structured owner-key identity before rendered compatibility/final-spelling paths and stop after complete owner-key misses instead of returning compatibility name ids.

Added focused LIR-linked coverage for `lir_aggregate_structured_name_id` through public lowering paths: global type refs prove complete owner-key hits produce verifier-consistent structured owner text/name-id mirrors instead of stale rendered text, and complete misses do not return or intern stale compatibility name ids; signature type refs prove incomplete/no-owner metadata still preserves the compatibility name-id path.

Reviewer result from `review/idea152_step6_route_review.md`: the retained Step 6 changes are aligned and not testcase-overfit, but Step 6 is incomplete after revert `323e73722` because `src/codegen/lir/hir_to_lir/types.cpp` still has `field_chain_nested_tag` returning `typespec_aggregate_compatibility_tag(...)` as a primary route for anonymous members.

## Suggested Next

Next coherent packet: repair or explicitly demote/document the `field_chain_nested_tag` compatibility route in `src/codegen/lir/hir_to_lir/types.cpp` under Plan Step 6. The packet should make structured owner/layout data the primary path when available, keep any compatibility path secondary with owner/limitation/removal-condition guidance, and avoid restoring the reverted over-broad field-chain slice.

## Watchouts

- The no-owner compatibility path remains intentionally live for incomplete metadata.
- Owner-key hits normalize aggregate-name mirrors to the structured owner tag; ABI fragments such as `ptr byval(%struct.X)` remain raw mirrors.
- `field_chain_nested_tag` is the remaining active LIR aggregate-helper compatibility route called out by review; accepting the current slice as complete Step 6 would be route drift.
- Current validation is failing in adjacent cpp positive signature type-ref coverage, so proof must be rerun after the Step 6 repair.

## Proof

Current `test_after.log` is failing, not passing. It records three cpp positive failures adjacent to LIR signature type refs:

- `cpp_positive_sema_template_angle_bracket_validation_cpp`: `LirFunction.signature_return_type_ref` missing matching `StructNameId`.
- `cpp_positive_sema_template_struct_advanced_cpp`: `LirFunction.signature_return_type_ref` missing matching `StructNameId`.
- `cpp_positive_sema_template_struct_nested_cpp`: `LirFunction.signature_param_type_refs` missing matching `StructNameId`.

Stale passing-proof claim replaced. After the next Step 6 repair, rerun the supervisor-selected proof or a supervisor-approved subset:

`cmake --build --preset default --target c4cll frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^frontend_lir_' --output-on-failure > test_after.log`
