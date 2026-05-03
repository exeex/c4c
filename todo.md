# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Frontend/HIR Test Fixture Tag Uses

## Just Finished

Step 1 classification of frontend/HIR test-fixture `TypeSpec::tag` residuals.
The latest deletion-probe files from `plan.md` split into these owned fixture
families:

- `tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`: two direct
  `TypeSpec::tag` writes, both intentional stale-rendered-spelling disagreement
  coverage. One case proves `record_def` wins over `tag`; one proves
  `tag_text_id` wins over `tag`.
- `tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`: two direct
  `TypeSpec::tag` writes, both intentional stale-rendered-spelling disagreement
  coverage for inherited static-member base lookup. One case uses `record_def`;
  one uses `tag_text_id` plus namespace context.
- `tests/frontend/frontend_parser_lookup_authority_tests.cpp`: direct
  `TypeSpec::tag` uses are mixed: stale-rendered disagreement fixtures for Sema
  completeness, consteval binding, alias-template NTTP carriers, parser type
  compatibility, typedef-chain resolution, and record-constructor
  classification; no-metadata rendered fallback fixtures; plus a few diagnostic
  detail/final spelling reads such as `resolved.tag ? resolved.tag : "<null>"`.
- `tests/frontend/frontend_parser_tests.cpp`: direct uses are mixed: parser
  final-spelling assertions for unresolved or tag-only type parsing,
  stale-rendered disagreement fixtures for member typedefs, alias templates,
  consteval lookup, qualified type metadata, and helper factories that inject a
  legacy rendered spelling into `TypeSpec` fixtures.
- `tests/frontend/frontend_hir_lookup_tests.cpp`: direct uses are mixed:
  stale-rendered disagreement fixtures for template origin keys, inherited
  static members, specialization/deduction, pending/canonical/mangling text,
  layout lookup, builtin query resolution, direct aggregate ownership, and
  member typedef readiness; rendered/no-metadata compatibility fixtures remain
  explicitly named.

Wider `tests/frontend` scan found direct `.tag` references in 36 files. Several
small `cpp_hir_*_metadata_test.cpp` files already use local SFINAE-style helper
wrappers around `TypeSpec::tag` and are likely not the first compile boundary
after field deletion. Many high-count `.tag` references in `frontend_hir_tests.cpp`
and `frontend_hir_lookup_tests.cpp` are `Node::tag` or `HirStructDef::tag`
layout/map keys rather than `TypeSpec::tag` field-removal blockers.

## Suggested Next

Execute Step 2 on the smallest latest-probe fixture family:
`tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`.

Owned files for the packet: `tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`
and `todo.md`.

Recommended migration: replace the two direct stale rendered `ts.tag = ...`
writes with a local compatibility helper that assigns the legacy field only
when `TypeSpec::tag` exists. Keep `record_def`, `tag_text_id`,
`namespace_context_id`, and qualifier metadata as the actual asserted semantic
carriers. Do not weaken the current stale-tag expectations while the field
still exists.

Recommended proof:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_template_type_arg_encoding_structured_metadata$"' > test_after.log 2>&1
```

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- `Node::tag` and `HirStructDef::tag` references are not the `TypeSpec::tag`
  field-removal target; classify them as final/layout/map spelling unless a
  deletion probe shows a `TypeSpec` compile error at that exact line.
- No-metadata rendered compatibility tests should move through explicit local
  helpers, not through a replacement semantic string field on `TypeSpec`.

## Proof

Classification-only packet. No code validation was required and no tests were
run. Read-only queries used:

```sh
rg -n "\.tag\b" tests/frontend
ctest --test-dir build -N | rg "cpp_hir_template_type_arg_encoding|cpp_hir_static_member_base_metadata|frontend_parser_lookup_authority|frontend_parser_tests|frontend_hir_lookup"
```
