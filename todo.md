# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 reran the temporary `TypeSpec::tag` deletion probe after migrating the
first `tests/frontend/frontend_parser_tests.cpp` boundary. The probe
temporarily disabled the field in `src/frontend/parser/ast.hpp` and ran:

```sh
cmake --build --preset default > test_after.log 2>&1
```

The previous `test_parser_parse_base_type_identifier_probes_use_token_spelling`
boundary no longer appears in the compile errors. The first remaining
`frontend_parser_tests.cpp` boundary moved to the direct `TypeSpec::tag` write
in `test_parser_dependent_typename_uses_local_visible_owner_alias` at line
1223. Later visible residuals remain in the same file and in
`frontend_parser_lookup_authority_tests.cpp`,
`cpp_hir_static_member_base_metadata_test.cpp`,
`frontend_hir_lookup_tests.cpp`,
`cpp_hir_member_typedef_binding_metadata_test.cpp`, and
`cpp_hir_nested_member_typedef_record_def_metadata_test.cpp`.

The probe edit was restored, and a normal `cmake --build --preset default`
passed.

## Suggested Next

Execute the next Step 2 fixture-migration packet on
`test_parser_dependent_typename_uses_local_visible_owner_alias` in
`tests/frontend/frontend_parser_tests.cpp`. Preserve its local-visible owner
alias contract by moving the direct legacy `alias_ts.tag` write behind an
explicit helper or structured carrier setup that compiles without
`TypeSpec::tag`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The `frontend_parser_tests` runtime failure from the previous packet is
  present both before and after that slice; do not treat it as a regression from
  fixture migration unless a new test failure appears.
- The next target should not reintroduce a replacement semantic string field on
  `TypeSpec`; use local compatibility helpers or structured metadata.

## Proof

Step 3 deletion probe failed as expected with `TypeSpec::tag` disabled and
recorded the next frontend/HIR fixture boundary in `test_after.log`. Restored
proof:

```sh
cmake --build --preset default
```
