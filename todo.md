# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the first `tests/frontend/frontend_parser_tests.cpp` deletion
boundary in `test_parser_parse_base_type_identifier_probes_use_token_spelling`.
The unresolved identifier fallback assertion no longer reads `TypeSpec::tag`
directly; it now asserts the structured `tag_text_id` carrier is valid and
resolves through `Parser::parser_text` to `ForwardDecl`, preserving the
parser-owned spelling contract.

The delegated proof compiled the target but did not pass because the
`frontend_parser_tests` binary fails later at an unrelated pre-existing
assertion:

```text
FAIL: namespace owner resolution should use the method owner TextId before rendered owner spelling
```

## Suggested Next

Commit this targeted boundary migration with the matching before/after guard
evidence, then run the temporary `TypeSpec::tag` deletion probe again. The
expected next local `frontend_parser_tests.cpp` boundary is the direct
`TypeSpec::tag` fixture write in
`test_parser_dependent_typename_uses_local_visible_owner_alias`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The targeted direct `unresolved_ts.tag` reads are removed, and the owned file
  compiles with the delegated build step.
- The `frontend_parser_tests` runtime failure is present both before and after
  this slice; do not treat it as a regression from this fixture migration.

## Proof

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
```

Result: blocked. The build completed, but `frontend_parser_tests` failed at the
namespace owner resolution assertion above. Log path: `test_after.log`.

Supervisor-side matching guard:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_before.log 2>&1
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS with the same pre-existing failure before and after
(`passed=0 failed=1 total=1`, no new failures).
