# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Reprove And Return To Parser Cleanup

## Just Finished

Step 6 reproof completed for the typed parser record identity bridge.

Focused parser/frontend proof passed against a matching refreshed baseline:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests|frontend_cxx_)' > test_after.log 2>&1`

Regression guard against matching focused `test_before.log` passed:
3 passed before, 3 passed after, no new failures.

Full-suite reproof passed against the accepted full-suite baseline:
3088 enabled tests passed before, 3088 enabled tests passed after, no new
failures. The disabled-test set remained the existing backend CLI trace/dump
tests 27, 28, and 41-50.

Supervisor overfit review found no supported-test downgrades, unsupported
expectation rewrites, or testcase-shaped shortcuts. The added coverage asserts
that stale rendered map state cannot override typed `TypeSpec::record_def`
authority in the converted paths.

## Suggested Next

Ask the plan owner to close or park idea 127 and resume or regenerate parent
parser cleanup idea 123 with `struct_tag_def_map` classified as a
compatibility/final-spelling mirror.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys as part of this bridge. The retained map still supports tag-only fallback,
testing hooks, rendered instantiation keys, and final spelling compatibility.

Preserve `TypeSpec::tag` as spelling, diagnostics, emitted text, and
compatibility payload; semantic record lookup should continue to flow through
`TypeSpec::record_def` where available.

Step 6 should be validation and lifecycle routing, not another semantic
conversion packet, unless reproof exposes a real covered-path gap.

## Proof

Supervisor validation:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests|frontend_cxx_)' > test_after.log 2>&1`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

`ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_after.log --allow-non-decreasing-passed`

Result: focused proof, full-suite proof, and both regression-guard comparisons
passed.
