# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the first fixture family,
`tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`, off direct
`TypeSpec::tag` writes. The two stale-rendered-spelling fixture writes now use
a local `set_legacy_tag_if_present` compatibility helper, so the test compiles
with or without the legacy member while still setting stale spelling when the
member exists.

The assertions still prove structured carriers win over stale rendered spelling:
`record_def` drives the struct type-argument encoding, and `tag_text_id` plus
qualified namespace metadata drives the typedef type-argument encoding.

## Suggested Next

Run Step 3's temporary `TypeSpec::tag` deletion probe to confirm the first
compile boundary moved past `cpp_hir_template_type_arg_encoding_test.cpp` and
classify the next frontend/HIR fixture family.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The next likely fixture family is
  `tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`, but the
  deletion probe should decide the actual next boundary.
- Keep no-metadata rendered compatibility tests moving through explicit local
  helpers, not through a replacement semantic string field on `TypeSpec`.

## Proof

Proof passed and wrote the canonical executor log to `test_after.log`:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_template_type_arg_encoding_structured_metadata$"' > test_after.log 2>&1
```
