# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Audit For Rename-Only Or Wrapper-Only Work

## Just Finished

Step 5 removed the Sema `lookup_symbol` rendered global/enum compatibility
authority route in `src/frontend/sema/validate.cpp`. Global and enum symbol
lookup now returns only structured-key results: direct reference keys,
using-value alias target keys, or the declaration's stored structured key for
exact rendered names that do not carry conflicting qualified owner metadata.
The old rendered global/enum fallback returns and now-unused structured-name
marker sets were deleted. No focused test edits were needed; the existing
lookup-authority tests and positive Sema subset covered the structured-vs-
rendered disagreement cases.

## Suggested Next

Continue Step 5 on the parser-side `*_from_spelling` route. Audit whether the
remaining spelling helpers are debug-only/compatibility mirrors or still act as
semantic authority, then remove or classify the exact no-carrier blocker.

## Watchouts

- Parser `*_from_spelling` remains the other Step 5 blocker after this Sema
  `lookup_symbol` route removal.
- `lookup_record_layout` remains a HIR-metadata/no-carrier blocker: removing
  the rendered `env.struct_defs->find(ts.tag)` lookup requires an env-carried
  structured HIR record owner/index or equivalent structured layout map outside
  this packet's owned Sema-only scope.
- Further HIR carrier work belongs under
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` before
  execution resumes on that boundary.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after removing the `lookup_symbol`
rendered-global/rendered-enum authority route:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
