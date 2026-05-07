Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Add Collision And Stale-Route Proof

# Current Packet

## Just Finished

Step 7: Add Collision And Stale-Route Proof added focused eager and deferred proof cases. `frontend_parser_lookup_authority_tests` now covers a same-suffix global collision where the rendered spelling names the wrong owner but qualifier/base TextId metadata selects the intended global. `frontend_hir_lookup_tests` now covers deferred member typedef resolution where the stale rendered owner tag has the same suffix and member name but the HIR owner-key/member-TextId carrier selects the correct typedef.

## Suggested Next

Supervisor can review and commit this Step 7 proof slice, then decide whether Step 8 helper-audit work should remove or further fence any remaining rendered qualified-name compatibility bridges.

## Watchouts

No implementation change was needed. The new cases are intentionally collision-shaped rather than fixture-shaped shortcuts: the stale rendered routes would select a wrong complete entity, while the structured eager and deferred paths keep selecting the intended owner/member identity.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. The corrected supervisor proof matched and ran 110 parser/HIR tests; all passed after the build completed.
