# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 inventory/probe for TypeSpec tag field removal.

Current direct-reference inventory command:
`rg -n "\.tag\b|->tag\b|decltype\([^\n]*tag" src/frontend/parser src/frontend/sema src/frontend/hir src/codegen tests/frontend`

Inventory summary:
- `src/frontend/parser/ast.hpp` still defines `TypeSpec::tag`.
- App-source references to `TypeSpec::tag` are mostly SFINAE-gated legacy
  display/final-spelling helpers in parser, Sema, HIR, and codegen shared
  helpers; deleting the field does not currently break `c4cll`.
- Raw `.tag` search also finds unrelated structured tags on HIR/LIR/backend
  records, so those are not all TypeSpec debt.
- The first compile blocker exposed outside `c4cll` is fixture/test debt in
  `tests/frontend/frontend_parser_tests.cpp`: direct writes/reads at lines
  7165, 7276, 7283, 7708, 7737, 7763, 7778, 7804, 7818, 7839, and 7882 after
  the field is removed.

## Suggested Next

Execute a Step 4-style test fixture packet for
`tests/frontend/frontend_parser_tests.cpp`: replace direct `TypeSpec::tag`
setup/assertions with structured metadata or SFINAE-compatible helper checks
without weakening stale-rendered-spelling disagreement coverage.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- `cmake --build build --target c4cll` passed while `TypeSpec::tag` was
  temporarily removed, so the immediate app-build boundary is not parser/Sema,
  HIR, or codegen source compilation.
- `frontend_parser_tests` is only a probe result here, not the delegated proof
  target; the failing probe log is `/tmp/typespec_tag_frontend_parser_tests_probe.log`.

## Proof

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran `cmake --build build --target c4cll > test_after.log 2>&1`; it passed.
- Ran `cmake --build build --target frontend_parser_tests > /tmp/typespec_tag_frontend_parser_tests_probe.log 2>&1`; it failed on direct
  `TypeSpec::tag` fixture references in `tests/frontend/frontend_parser_tests.cpp`.
- Restored the field and ran `cmake --build build --target c4cll >> test_after.log 2>&1`; it passed, leaving the repository buildable.

Canonical proof log: `test_after.log`.
