# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Final Frontend Coverage And Regression Check

## Just Finished

Step 5 passed post-blocker-fix review in
`review/step5_post_blocker_fixes_audit.md`: route alignment is on track, no
blocking testcase-overfit was found, and the reviewer recommends continuing
into Step 6 with broader validation before acceptance.

The prior parser `find_known_fn_name_key_from_spelling` /
`intern_known_fn_name_key_from_spelling` wrapper blocker is fixed. Semantic key
construction from separate `TextId` plus namespace-context carriers now goes
through `qualified_key_in_context`; the only retained rendered-string parsing
is visibly named `*_compatibility_key_from_rendered_qualified_spelling` and is
classified as a no-segment-carrier compatibility boundary for single `TextId`
values whose text already contains `::`.

The prior Sema rendered global/enum compatibility return blocker is fixed for
Step 5: returned authority now comes through structured keys, though
`lookup_symbol(const std::string& name, const Node* reference)` remains in
Step 6 watch scope because its public shape is still string-taking and the
no-reference/no-qualified-metadata path can still use rendered spelling to
find a structured key.

## Suggested Next

Execute Step 6 validation with a fresh canonical proof artifact. Minimum
review-required scope:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`

Because idea 139 has many commits since the active contract checkpoint and has
touched several parser/Sema lookup families, Step 6 should also run a broader
frontend CTest pass or the repo's regression guard before final acceptance.

## Watchouts

- The retained no-carrier parser bridge is classified as technical debt, not
  Step 5 failure: splitting those callers requires carrying qualifier segment
  TextIds, a `QualifiedNameRef`, namespace context, or `QualifiedNameKey`
  before they reach `qualified_key_in_context`.
- `lookup_record_layout` remains a parked HIR-metadata/no-carrier blocker, not
  active idea 139 parser/Sema scope: removing the rendered
  `env.struct_defs->find(ts.tag)` lookup requires an env-carried structured
  HIR record owner/index or equivalent structured layout map.
- Parser const-int / HIR NTTP compatibility remains a parked HIR metadata
  blocker, not active idea 139 scope: deleting the rendered-name
  `eval_const_int(..., const std::unordered_map<std::string, long long>*)`
  compatibility overload requires HIR `NttpBindings` metadata migration.
- Further HIR carrier work belongs under
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` before
  execution resumes on that boundary.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Step 5 executor-reported proof after replacing the parser-side
`*_from_spelling` semantic wrapper route:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Reported result: build completed successfully; CTest passed `886/886` matched
tests. The Step 5 audit notes that the root `test_after.log` artifact was not
present during review, so Step 6 must produce a fresh canonical
`test_after.log` before acceptance.
