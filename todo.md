# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the next `frontend_parser_tests.cpp` member typedef stale tag
fixture write in `test_parser_member_typedef_suffix_prefers_record_definition`.
The targeted `alias_ts.tag = arena.strdup("StaleBox")` assignment now goes
through the local `set_legacy_tag_if_present` compatibility helper, while
preserving `record_def = real_owner`, the stale owner setup, and the assertion
that member typedef suffix lookup prefers `record_def` before stale rendered
tags.

## Suggested Next

Rerun the temporary `TypeSpec::tag` deletion probe and classify the next
frontend/HIR fixture boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

The delegated proof command was:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
```

The build completed. `frontend_parser_tests` still failed with the known
outside-packet assertion:

```text
FAIL: namespace owner resolution should use the method owner TextId before rendered owner spelling
```

Proof log: `test_after.log`.

Supervisor-side matching guard passed with the same known failure before and
after:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS (`passed=0 failed=1 total=1`, no new failures).
