# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for the first TypeSpec tag deletion blocker
in `tests/frontend/frontend_parser_tests.cpp`, including the stale
tag-only-record assertion that was blocking `frontend_parser_tests`.

The direct fixture reads/writes at the prior deletion-probe sites now use the
local SFINAE-compatible `set_legacy_tag_if_present` helper or TextId-backed
spelling checks. A current direct-reference check for this file:
`rg -n "\.tag\b|->tag\b|decltype\([^\n]*tag" tests/frontend/frontend_parser_tests.cpp`
now reports only the SFINAE helper body.

The former tag-only record fixture now asserts the parser's current structured
contract: `struct Forward after;` preserves source spelling through
`tag_text_id` and carries an incomplete parser-owned `NK_STRUCT_DEF` record
identity instead of expecting `record_def == nullptr`.

With `const char* tag` temporarily removed from `TypeSpec`, `cmake --build build
--target frontend_parser_tests` now passes, so this file is no longer the
frontend parser test compile boundary for deleting the field.

## Suggested Next

Move the TypeSpec tag deletion probe to the next target that still has direct
fixture references under `tests/frontend/`; a broad inventory outside
`frontend_parser_tests.cpp` still finds direct `.tag` sites in focused metadata
tests and larger lookup/HIR suites.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- The deletion probe did not expose a next compile blocker inside
  `frontend_parser_tests`.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target frontend_parser_tests c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Result: passed. The earlier runtime assertion
`tag-only struct TypeSpec should not synthesize typed record identity` was
updated to the current incomplete-record structured identity contract before
the final proof run.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran `cmake --build build --target frontend_parser_tests`; it passed and got
  past the previous direct fixture references.
- Restored the field and ran `cmake --build build --target frontend_parser_tests
  c4cll`; it passed, leaving the repository buildable with the field present.
