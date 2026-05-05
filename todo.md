# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the remaining delegated C++ parse timeout:
`cpp_eastl_vector_parse_recipe` no longer hangs while parsing EASTL
`numeric_limits.h`. Dependent-typename owner-prefix reparses now go through
`parse_injected_base_type()`, and that helper guarantees injected token streams
end with `EndOfFile` so parse loops cannot pin on a terminal synthetic `;`.
No tests were weakened and no rendered-tag consumer fallback was added.

## Suggested Next

Continue Step 6 against the refreshed full-suite non-C++-route leftovers in
the C external suites: incomplete C record typedef/object families
(`RenderInfo`, `DB_LSN`, `DB_TXNLIST`), C struct mirror/name disagreement in
GEP/call metadata, one C function redeclaration conflict, one C field lookup
miss, and two C runtime abort/segfault cases.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The repaired timeout came from open-coded injected reparses of dependent
  template owner prefixes. Future injected parser routes should use
  `parse_injected_base_type()` or otherwise include an EOF token and restore
  the full parser token stream.
- Rejected baseline candidate: accepting the visible lexical alias
  type-argument failure as baseline drift is not valid because
  `frontend_parser_tests` contains the intended non-fabrication guard and the
  regression is local to parser TypeSpec metadata production.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Accepted proof is in `test_after.log`:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_declarator_deferred_owner_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_declarator_deferred_owner_structured_metadata|cpp_eastl_(vector|utility|memory_uses_allocator)_parse_recipe)$'`.

The delegated subset ran 5 tests and all passed. `cpp_eastl_vector_parse_recipe`
passed in 4.65s; EASTL utility and memory recipes plus
`frontend_parser_tests` and
`cpp_hir_parser_declarator_deferred_owner_structured_metadata` stayed green.

Refreshed full-suite evidence was appended to `test_after.log` with
`ctest --test-dir build -j --output-on-failure`: 3012/3023 passed, 11 failed,
all in C external suites. Failing families were C incomplete object types,
C struct mirror/name disagreement, one C conflicting function type, one C
field lookup miss, and two C runtime abort/segfault cases.
