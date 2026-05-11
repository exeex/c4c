Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

# Current Packet

## Just Finished

Step 6 validation blocker repaired stale owned test fixtures that still wrote
or asserted removed `TypeSpec::tag` compatibility payloads.

`backend_prepare_structured_context_test` now seeds `TypeSpec::tag_text_id` for
the structured signature drift parameter. `frontend_hir_tests` now builds its
record, typedef, member lookup, template, destructor, constructor, and
out-of-class-method fixtures through current structured owner/member metadata
instead of retired rendered-tag carriers, while preserving the stale-vs-real
owner assertions.

## Suggested Next

Resume Step 6 proof or supervisor review now that the default build blocker in
the owned test fixtures is cleared.

## Watchouts

- The frontend fixture repairs intentionally keep stale rendered names present
  where useful, but move the authoritative lookup path to `tag_text_id`,
  `record_def`, owner keys, qualifier TextIds, member fields, and structured
  NTTP metadata.
- No production files or tests outside the delegated owned set were touched.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_prepare_structured_context_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_structured_context|frontend_hir_tests)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 2/2 selected tests
passed.
