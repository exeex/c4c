# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 reran the temporary `TypeSpec::tag` deletion probe after the
`tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp` fixture helper
migration. The probe temporarily disabled the field in
`src/frontend/parser/ast.hpp` and ran:

```sh
cmake --build --preset default > test_after.log 2>&1
```

The migrated `cpp_hir_template_type_arg_encoding_test.cpp` target compiled
under the probe and no longer appears in the `TypeSpec::tag` failure list. The
first remaining compile boundary is now
`tests/frontend/frontend_parser_tests.cpp`, starting at direct `TypeSpec::tag`
reads in `test_parser_parse_base_type_identifier_probes_use_token_spelling`
and then many parser fixture writes/reads. Other visible residual fixture
families behind it include
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`,
`tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`,
`tests/frontend/frontend_hir_lookup_tests.cpp`, and
`tests/frontend/cpp_hir_member_typedef_binding_metadata_test.cpp`.

The probe edit was restored, and a normal `cmake --build --preset default`
passed.

## Suggested Next

Execute the next Step 2 fixture-migration packet on a narrow
`tests/frontend/frontend_parser_tests.cpp` family rather than reopening codegen
or the already-migrated template type-arg test. Start with the earliest probe
boundary, `test_parser_parse_base_type_identifier_probes_use_token_spelling`,
and preserve its final/token spelling contract with a local legacy-tag read
helper or structured `tag_text_id` assertion as appropriate.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- `tests/frontend/cpp_hir_static_member_base_metadata_test.cpp` is still a
  visible deletion-probe residual, but it is not the first boundary after this
  probe.
- Keep no-metadata rendered compatibility tests moving through explicit local
  helpers, not through a replacement semantic string field on `TypeSpec`.
- `tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp` should not be
  revisited unless a fresh probe reports a new failure in that file.

## Proof

Previous Step 2 proof:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_template_type_arg_encoding_structured_metadata$"' > test_after.log 2>&1
```

Step 3 deletion probe failed as expected with `TypeSpec::tag` disabled and
recorded the next frontend/HIR fixture boundary in `test_after.log`. Restored
proof:

```sh
cmake --build --preset default
```
