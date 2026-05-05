# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`.

The lookup-authority fixture cluster no longer contains direct test-body
`TypeSpec::tag` reads or writes. Legacy rendered spelling setup and diagnostics
now go through SFINAE-gated test helpers, so the stale-rendered disagreement
coverage remains active while `TypeSpec::tag` exists but the file also compiles
under a controlled field-deletion probe.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target frontend_parser_lookup_authority_tests`. The
focused target built successfully, so there is no remaining deletion-probe
blocker in this fixture cluster. Restored the field before final proof.

## Suggested Next

Run the next controlled `TypeSpec::tag` deletion probe at the supervisor-chosen
scope, likely the broader build, to identify the next compile blocker outside
`frontend_parser_lookup_authority_tests.cpp`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- `frontend_parser_lookup_authority_tests.cpp` still has SFINAE helper
  references to `.tag` by design; no direct fixture-body `.tag` references
  remain.
- The focused deletion probe did not expose the next blocker. A broader
  deletion probe is needed to find the next migration packet.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target frontend_parser_lookup_authority_tests c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_lookup_authority_tests$'`

Result: passed. `frontend_parser_lookup_authority_tests` is green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran `cmake --build build --target frontend_parser_lookup_authority_tests`;
  the focused target passed with the field removed, so no remaining blocker was
  found in this target.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
