# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Audit For Rename-Only Or Wrapper-Only Work

## Just Finished

Step 4 is complete for idea 139's parser/Sema-owned route and parked at the
HIR metadata boundary identified by
`review/step4_post_consteval_layout_blocker_review.md`. Sema
`lookup_record_layout` in `src/frontend/sema/consteval.cpp` still cannot
remove its rendered `TypeSpec::tag` route inside idea 139: the only
record-layout carrier exposed in `ConstEvalEnv` is rendered-keyed
`struct_defs`, while the structured HIR record owner index lives on
`hir::Module` and is not carried into consteval layout lookup.

The remaining `lookup_record_layout` work is routed to
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`: add a structured
HIR record-layout carrier to `ConstEvalEnv`, such as an owner-key lookup
channel backed by HIR `struct_def_owner_index` or an equivalent structured
record identity to `HirStructDef` layout map. Do not expand idea 139 Step 4
with that HIR carrier work.

## Suggested Next

Proceed with Step 5 audit of the idea 139 diff for helper-only renames,
wrapper-only rendered lookup preservation, fallback/legacy semantic routes,
string/string_view semantic APIs, and expectation downgrades. If that audit
finds only the parked HIR `lookup_record_layout` carrier gap, leave it routed
to idea 140 and do not reopen Step 4.

## Watchouts

- This lifecycle packet intentionally did not edit implementation files, tests,
  or `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`.
- `lookup_record_layout` remains a HIR-metadata/no-carrier blocker: removing
  the rendered `env.struct_defs->find(ts.tag)` lookup requires an env-carried
  structured HIR record owner/index or equivalent structured layout map outside
  this packet's owned Sema-only scope.
- Further HIR carrier work belongs under
  `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` before
  execution resumes on that boundary.
- The AST query attempt with `c4c-clang-tool-ccdb` could not load
  `src/frontend/sema/consteval.cpp` from `build/compile_commands.json`; local
  focused reads were used for the narrow inspection.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after the `lookup_record_layout` blocker
classification:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
