Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Final Compatibility Boundary Review

# Current Packet

## Just Finished

Step 8: Final Compatibility Boundary Review re-scanned the parser/shared/Sema/HIR surfaces for rendered qualified-name authority. `find_compatibility_key_from_rendered_qualified_spelling()` remains only as an explicitly labeled compatibility bridge for legacy rendered qualified `TextId` carriers after structured metadata paths, and `split_qualified_name_scope()` is fenced in `src/shared/qualified_name_table.hpp` as a legacy rendered-spelling helper rather than semantic lookup authority. The one-`TextId` lookup APIs inspected in parser core still reject qualified spellings at their public lookup boundaries.

## Suggested Next

Supervisor can review this Step 8 audit slice and decide whether the active runbook is ready for plan-owner close/deactivation review.

## Watchouts

Attempting to remove the rendered qualified-name compatibility bridge from the central parser key builder caused qualified template/HIR compatibility regressions in the delegated subset, so this slice kept the bridge and fenced it explicitly instead. Further removal needs a separate migration packet that replaces the remaining still-migrating template paths with fully structured carriers before deleting the bridge.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_|cpp_positive_sema_'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. The supervisor-selected subset ran 995 tests; all passed after the build completed.
