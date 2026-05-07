Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Eager Sema Qualified Lookup Through Domain Tables

# Current Packet

## Just Finished

Step 4: Route Eager Sema Qualified Lookup Through Domain Tables tightened Sema TypeSpec equivalence so rendered qualified-name compatibility is gated behind the absence of structured TextId/namespace metadata, while preserving explicit no-metadata rendered qualified compatibility. Added focused frontend parser tests that reject stale rendered TypeSpec names when structured identity differs and keep legacy rendered qualifier comparison when no structured metadata is available.

## Suggested Next

Supervisor can review and commit this Step 4 Sema TypeSpec equivalence slice, then choose the next qualified lookup authority packet.

## Watchouts

`has_any_text_name_metadata` now means actual structured identity metadata: namespace context, base TextId, or usable qualifier TextIds. Raw rendered qualifier spelling and the global-qualified bit still participate in the rendered compatibility path when no structured metadata exists, but they no longer block that path by themselves.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_member_typedef_origin_binding_structured_metadata)$'; } > test_after.log 2>&1`

Corrected proof passed. `test_after.log` is the canonical executor proof log. CTest matched and ran `frontend_parser_lookup_authority_tests`, `frontend_parser_tests`, `cpp_hir_sema_canonical_symbol_structured_metadata`, `cpp_hir_parser_support_residual_structured_metadata`, and `cpp_hir_member_typedef_origin_binding_structured_metadata`; all passed after the build completed.
