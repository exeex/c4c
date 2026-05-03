# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the next stale rendered tag fixture write in
`test_parser_dependent_typename_owner_alias_prefers_record_definition`.
The direct `alias_ts.tag = arena.strdup("StaleBox")` assignment now goes
through the local `set_legacy_tag_if_present` compatibility helper while
preserving `record_def = real_owner` and the stale owner setup.

The delegated proof command rebuilt the test binary, then
`frontend_parser_tests` failed at the known unrelated assertion:

```text
FAIL: namespace owner resolution should use the method owner TextId before rendered owner spelling
```

## Suggested Next

Rerun the temporary `TypeSpec::tag` deletion probe and classify the next
frontend/HIR fixture boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The known `frontend_parser_tests` namespace-owner assertion is outside this
  packet and should not be repaired as part of narrow stale tag fixture writes.

## Proof

Delegated proof command:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
```

Result: build completed; `frontend_parser_tests` failed with the known
namespace-owner assertion. Proof log: `test_after.log`.

Supervisor-side matching guard:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_before.log 2>&1
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS with the same pre-existing failure before and after
(`passed=0 failed=1 total=1`, no new failures).
