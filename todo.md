# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 repaired the `frontend_parser_tests` runtime failure where visible
lexical alias type-argument parsing fabricated typedef TextId metadata. The
shared `parse_base_type()` typedef handoff now preserves only real qualified
spelling metadata across a resolved typedef payload, so an unqualified visible
alias to `int` no longer leaves the target typedef TextId on the concrete
argument type.

The same repair keeps resolved typedef payloads with intrinsic TextId identity
complete by filling the current namespace context when the payload has a
`tag_text_id` but no context. This preserves the structured carrier/equivalence
fixtures that compare TextId metadata instead of rendered spelling.

The alias-of-alias member typedef fixture moved its stale rendered owner write
behind `set_legacy_tag_if_present()`, and the consteval lookup fixture now
constructs explicit structured/TextId carriers for the metadata paths it
asserts instead of relying on rendered-name side maps.

## Suggested Next

Run the next Step 2 deletion probe to find the next direct fixture `TypeSpec::tag`
residual after the migrated alias-of-alias cluster, then migrate that fixture
without changing its structured metadata assertion.

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

## Proof

Proof output is recorded in `test_after.log`.

```text
cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: passed.
