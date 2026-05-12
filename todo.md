# Current Packet

Status: Active
Source Idea Path: ideas/open/170_string_authority_regression_guard.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Integrate Developer Workflow

## Just Finished

Step 4 integrated the local string-authority guard into the developer workflow
without changing the scanner roots or adding call-site scanning.

Changed files:

- `scripts/string_authority_guard.py`
- `tests/CMakeLists.txt`
- `todo.md`

Implementation notes:

- `tests/CMakeLists.txt` now registers `string_authority_guard` and
  `string_authority_guard_self_test` CTest entries under
  `workflow;string_authority`, plus a `string_authority_guard` build target that
  runs both Python commands from the repo root.
- `scripts/string_authority_guard.py --help` now documents the direct Python
  commands, the CTest regex, and reviewer expectations for exact classification
  entries with owner, domain, category, reason, removal condition, and evidence.
- the help text explicitly keeps entries exact by `path` + `symbol` and says not
  to broaden scanner roots or ordinary `.find(name)` call-site scanning.

## Suggested Next

Step 5 should prove guard coverage and non-disruption. A good next packet is to
run the supervisor-selected build or CTest subset after this workflow
integration, including `ctest --test-dir build -R string_authority_guard
--output-on-failure`, and record whether failures are guard issues,
pre-existing unrelated failures, or lifecycle blockers.

## Watchouts

- The current guard reports 235 classified declaration-level hits after the
  stricter Step 3 inventory expansion.
- The first implementation deliberately avoids ordinary `.find(name)` call-site
  scanning; widening into expression use sites should be a separate
  re-inventory packet with new exact classifications.
- The new CTest entries require a configured build tree that has seen the
  updated `tests/CMakeLists.txt`; re-run `cmake --preset default` or an
  equivalent configure step if `ctest -R string_authority_guard` does not list
  them.

## Proof

Step 4 proof:

- `python3 scripts/string_authority_guard.py > test_after.log 2>&1`
- `python3 scripts/test_string_authority_guard.py`
- `cmake --preset default`
- `ctest --test-dir build -R string_authority_guard --output-on-failure`
- `cmake --build --preset default --target string_authority_guard`
- `git diff --check`

All commands passed. The direct guard proof writes the canonical executor proof
log to `test_after.log`; the CTest regex ran both `string_authority_guard` and
`string_authority_guard_self_test`.
