# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the first remaining direct frontend parser fixture cluster
after the alias-of-alias handoff. The three
`typespec_mentions_template_param` fixtures and the adjacent
`encode_template_arg_debug_ref` fixture no longer write `TypeSpec::tag`
directly; stale rendered spellings now go through
`set_legacy_tag_if_present()`, and the no-rendered-spelling fixture relies on
zero-initialized `TypeSpec` state while keeping its structured
`template_param_text_id` assertion.

## Suggested Next

Migrate the next Step 2 deletion-probe boundary in
`test_type_binding_equivalence_uses_deferred_member_text_id_authority`, starting
at `tests/frontend/frontend_parser_tests.cpp:7132`, without weakening the
deferred-member TextId equivalence assertions.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` target build plus ctest now passes; the
  runtime failure was in the unqualified typedef metadata handoff, not a
  template-argument special case.
- An intrinsic structured consteval lookup miss remains authoritative in the
  current sema contract; the fixture now uses TextId fallback only when the
  structured lookup map is unavailable.
- The migrated tag-only fallback fixture intentionally keeps no `record_def`;
  the compatibility map remains the intended authority for the 1/2/1 layout
  results.
- The incomplete-declaration helper now depends on `record_def` as the real
  authority while keeping `Shared` rendered-map disagreement coverage through
  the stale `struct_tag_def_map` entry.
- The stale-rendered parameter disagreement coverage in the migrated fixture
  remains in `info.param_names = {"StaleRenderedT"}` versus
  `info.param_name_text_ids = {param_text}`.
- The no-rendered-param-name fixture intentionally omits `param_names`; keep
  that structured-only behavior intact.
- `src/frontend/parser/ast.hpp` was only changed for the temporary deletion
  probe and has no lasting diff.
- The next deletion probe now first reports direct `TypeSpec::tag` access at
  `tests/frontend/frontend_parser_tests.cpp:7132`; later residuals remain in
  the same file and were intentionally left for later packets.

## Proof

Proof output is recorded in `test_after.log`.

```text
cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
temporary deletion probe: remove TypeSpec::tag from src/frontend/parser/ast.hpp, then cmake --build build --target frontend_parser_tests >> test_after.log 2>&1
restore src/frontend/parser/ast.hpp, then cmake --build build --target frontend_parser_tests >> test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: normal build and ctest passed before and after restoration; deletion
probe failed at the expected next direct fixture boundary,
`tests/frontend/frontend_parser_tests.cpp:7132`.
