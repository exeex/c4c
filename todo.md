# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes

## Just Finished

Continued Step 3.3 by narrowing `lookup_symbol()` global/enum rendered
compatibility after structured metadata misses. Rendered global and enum
fallbacks now reject stale reference metadata unless the remaining compatibility
binding has no structured metadata or the binding's own structured global
metadata agrees with its rendered spelling.

Added focused `frontend_parser_lookup_authority_tests` coverage for stale
rendered global and enum lookups where the reference carries structured
metadata but the rendered binding cannot satisfy that structured key.

## Suggested Next

Continue Step 3.3 by deciding whether the local-symbol rendered fallback after
a structured local miss can be removed without breaking textless injected
predefined locals such as `__func__`, or route a producer packet to give those
locals structured metadata first.

## Watchouts

- The broad proof still contains the pre-existing `frontend_parser_tests`
  failure: `record-body using member typedef writer should register a direct
  record/member key`. Supervisor-side matching before/after validation confirms
  this packet did not introduce that failure.
- An over-strict first cut also broke namespace/global using-declaration
  bridges; the retained `structured_global_self_names_` guard is required so a
  same-spelling structured global can still satisfy those imports while stale
  rendered globals whose metadata names another symbol remain rejected.
- `c4c-clang-tools` was not needed for this packet; the candidate route was
  localized in `src/frontend/sema/validate.cpp::lookup_symbol()`.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_positive_sema_)' >> test_after.log 2>&1`

Result: completed with the same pre-existing `frontend_parser_tests` failure
present in both before and after logs. `test_after.log` contains a fresh
successful build, passing `frontend_parser_lookup_authority_tests`, and 884/884
passing `cpp_positive_sema_` tests. The shared failure is `FAIL: record-body
using member typedef writer should register a direct record/member key`.

Supervisor-side regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with before=885 passed/1 failed/886 total and after=885
passed/1 failed/886 total; no new failures.
