# 145 Reopen Cleanup Boundary Review

Date: 2026-05-07
Source idea: `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`

## Result

No remaining closure blocker found for the reopened cleanup scope.

## Scope Reviewed

- `src/frontend/parser/impl/support.cpp`
- `tests/frontend/cpp_hir_parser_support_residual_metadata_test.cpp`
- full-suite `test_before.log` / `test_after.log`

## Boundary Judgment

The reopened blocker was the incomplete `TypeSpec::record_def` compatibility
path in `resolve_record_type_spec()`. That path previously scanned
`struct_tag_def_map` and completed an incomplete parser record by matching
unqualified `TextId`, `name`, or `unqualified_name`.

The cleanup removes that parser-map completion branch. A structured
`record_def` is now preserved as the direct provisional carrier, including when
it is incomplete. TextId-less legacy compatibility below that branch remains
separate and explicitly outside this cleanup.

The focused regression test now proves that an incomplete structured
`record_def` does not complete through a stale rendered-tag parser map entry.

## Proof

Focused proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_parser_support_residual_metadata_test|frontend_parser_tests|frontend_parser_lookup_authority_tests)$'
./build/tests/frontend/cpp_hir_parser_support_residual_metadata_test
```

Result: passed. The `cpp_hir_parser_support_residual_metadata_test` executable
is not registered as a CTest test, so it was run directly.

Full-suite proof:

```bash
ctest --test-dir build -j --output-on-failure > test_after.log
```

Result: 3023/3023 tests passed.

Regression guard:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS.
