# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/frontend_hir_lookup_tests.cpp`.

The remaining direct `TypeSpec::tag` fixture setup in this target now goes
through SFINAE-gated helpers. Stale-rendered-spelling disagreement coverage
continues to run while `TypeSpec::tag` exists, and the tag-only nominal
specialization compatibility case is skipped only when the field is absent.
Non-TypeSpec HIR struct `tag` fields, map keys, owner strings, and Node tags
were left as direct fixture data.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target frontend_hir_lookup_tests`. The focused target
built successfully with the field removed, so no remaining deletion-probe
blocker was found inside this owned target. Restored the field before final
proof, leaving no `ast.hpp` diff.

## Suggested Next

Run the supervisor-chosen next broad TypeSpec::tag deletion boundary check and
delegate the next fixture target if another direct test debt bucket appears.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- `tests/frontend/frontend_hir_lookup_tests.cpp` still has SFINAE helper
  references to `.tag` by design, and direct `.tag` writes remain only for
  non-TypeSpec fixture data such as `c4c::hir::HirStructDef::tag`.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target frontend_hir_lookup_tests c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`

Result: passed. `frontend_hir_lookup_tests` is green in `test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran
  `cmake --build build --target frontend_hir_lookup_tests`; the focused target
  built successfully with the field removed, so no next blocker appeared inside
  this target.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
