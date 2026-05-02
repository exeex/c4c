# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Final Frontend Coverage And Regression Check

## Just Finished

Step 6 fixed the final `frontend_parser_tests` validation failure:
`enum constant lookup should reject stale qualified rendered names after a
structured enum miss`.

The failure reproduced under the delegated Step 6 proof command. Sema
`lookup_symbol` already suppressed rendered compatibility re-entry for
qualified-name segment metadata, but a leading `::` without owner segments was
not counted as qualified structured metadata. That let
`structured_enum_const_keys_by_name_` rescue a stale rendered enum spelling
after the structured enum lookup missed.

`src/frontend/sema/validate.cpp` now treats `reference->is_global_qualified`
as qualified structured metadata for the rendered-key guard. The fix preserves
structured-key authority and does not weaken the stale-qualified enum
expectation.

Review `review/step6_post_global_qualifier_fix_review.md` judged the Step 6
global/enum lookup route on track, with no new testcase-shaped shortcut,
unsupported expectation downgrade, or helper-only wrapper route found. The
review also found this file's current-step metadata stale and noted that final
acceptance still needs a fresh Step 6 validation proof because the reported
`test_after.log` is not present.

## Suggested Next

Supervisor should rerun the Step 6 validation command and produce the final
canonical proof log before deciding whether the active runbook can close or
needs a remaining metadata-blocker split.

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
- Existing untracked `review/step5_*.md` artifacts were also left untouched.

## Proof

Step 6 delegated proof to regenerate:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Current proof state: pending. The previous passing Step 6 proof was rolled into
`test_before.log`, and `test_after.log` is absent, so final Step 6 acceptance
requires fresh validation output.
