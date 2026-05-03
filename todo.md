# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the direct `alias_ts.tag = arena.strdup("Box")` write in
`test_parser_dependent_typename_uses_local_visible_owner_alias` behind the
local `set_legacy_tag_if_present` compatibility helper. The fixture still seeds
the stale legacy owner spelling while `TypeSpec::tag` exists, and it preserves
the local-visible owner alias contract through `record_def`.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to confirm this boundary
moved and classify the next frontend/HIR fixture residual.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The delegated `frontend_parser_tests` proof still fails at the known
  namespace-owner assertion:
  `FAIL: namespace owner resolution should use the method owner TextId before rendered owner spelling`.
  The build step completed before that runtime failure.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The next target should not reintroduce a replacement semantic string field on
  `TypeSpec`; use local compatibility helpers or structured metadata.

## Proof

Delegated proof command:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
```

Result: build completed; `frontend_parser_tests` failed with the known
namespace-owner assertion above. Proof log: `test_after.log`.

Supervisor-side matching guard:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_before.log 2>&1
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_parser_tests$"' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS with the same pre-existing failure before and after
(`passed=0 failed=1 total=1`, no new failures).
