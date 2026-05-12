# String Authority Regression Guard

Status: Closed
Created: 2026-05-11
Closed: 2026-05-12

Depends On:
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- `ideas/closed/168_compatibility_bridge_retirement.md`

Parent Ideas:
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Add a lightweight regression guard so new semantic string authority does not
silently re-enter the compiler.

The guard should not ban `std::string`. It should flag suspicious new
string-keyed semantic maps or broad rendered-name fallback APIs and require
classification as structured authority, compatibility bridge, display/output,
diagnostic/debug, or route-local identity.

## Why This Idea Exists

The cleanup series replaces a long-lived habit: using rendered strings as the
default identity carrier. Without a guard, future patches can easily add a new
`unordered_map<string, ...>` or `find(name)` path beside structured domains and
undo the migration gradually.

## In Scope

- Add a repo-native check, script, test, or documented review gate that scans
  for suspicious string-authority patterns.
- Seed the allowlist from idea 167 and idea 168 classifications.
- Make the check distinguish likely semantic maps from obvious output,
  diagnostic, route-local, and false-positive strings.
- Require new allowlist entries to state owner, domain, and why the string is
  not ordinary semantic authority.
- Integrate the guard into an appropriate test or developer workflow without
  making normal string use painful.

## Out Of Scope

- A perfect static analyzer.
- Banning strings from diagnostics, printers, route-local generated names, or
  final output.
- Reopening all compatibility bridges by hand during the guard work.
- Blocking urgent backend/frontend work on unrelated false positives.

## Acceptance Criteria

- A repeatable guard exists and can be run locally.
- The guard catches representative semantic string-keyed lookup patterns.
- Existing accepted strings are allowlisted or classified with owner/domain.
- Adding a new suspicious string map requires an explicit classification.
- Documentation explains how to classify valid display, diagnostic,
  compatibility, and route-local cases.

## Closure Notes

Closed after Step 5. The implemented guard is intentionally declaration-level
only, scans configured roots, and requires exact path plus symbol
classifications for accepted hits. Ordinary `.find(name)` call-site scanning is
out of scope for this idea.

Implemented artifacts:
- `scripts/string_authority_classifications.json`
- `scripts/string_authority_guard.py`
- `scripts/test_string_authority_guard.py`
- CTest entries and `string_authority_guard` build target in
  `tests/CMakeLists.txt`

Closure proof:
- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(string_authority_guard|backend_|backend_codegen_route_|positive_sema_inline_backend_coord_c$)' > test_after.log 2>&1`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: close accepted. The Step 5 broader proof passed with 112/112 runnable
tests and 12 disabled MIR trace tests not run; the monotonic regression guard
reported no new failures.

## Reviewer Reject Signals

- The guard fails because the repo contains ordinary diagnostic/output strings.
- The allowlist has unexplained entries.
- The guard is so broad that engineers will routinely bypass it.
- The guard misses obvious new semantic `unordered_map<string, ...>` additions
  in covered compiler domains.
