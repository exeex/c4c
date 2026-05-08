Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed Plan Step 6 by retiring rendered compatibility recovery in `lookup_structured_layout` after a complete aggregate owner key exists but misses.

`lookup_structured_layout` now treats `typespec_aggregate_owner_key(ts, mod)` as authoritative: it returns the structured HIR/LIR layout path when the owner key hits, stops immediately when the complete owner key misses, and consults rendered compatibility tags only when no complete owner metadata exists. Focused LIR-linked coverage in `frontend_lir_global_type_ref_test` directly proves complete owner-key hits select the structured HIR/LIR layout, stale rendered HIR/LIR layout compatibility is rejected after a complete owner-key miss, and incomplete metadata still preserves rendered compatibility.

## Suggested Next

Next coherent packet: continue Step 6 by retiring the next LIR aggregate helper rendered-compatibility fallback identified by supervisor review, or move to the next plan step if no Step 6 fallback remains.

## Watchouts

- The focused direct test lives in `frontend_lir_global_type_ref_test` because it links `c4c_codegen`; `frontend_hir_lookup_tests` still cannot call LIR implementation symbols directly.
- The incomplete-metadata compatibility branch uses an invalid qualifier segment to keep owner metadata incomplete while still exercising `tag_text_id` rendered compatibility.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target c4cll frontend_hir_lookup_tests frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R '^(frontend_hir_lookup_tests|frontend_lir_)' --output-on-failure > test_after.log`
