# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 6 first narrow repair packet complete: added the deprecated,
  searchable `LirTypeRef::parsed_typed_call_argument_text` compatibility
  boundary for parsed/re-owned typed-call argument text. Its only selected
  warning sites are `own_lir_typed_call_args` and the empty-mirror fallback in
  `lir_call_arg_type_refs`; both document that dynamic aggregate, vector,
  struct, and function spellings remain runtime-text compatibility paths.

## Suggested Next

- Step 6: inspect the two warned parsed/re-owned typed-call sites and select
  the next bounded closed-set migration or retained compatibility boundary.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- The global `const char*` warning experiment was rejected because it emitted
  widespread unrelated warnings; the repaired route must remain local.
- Keep dynamic aggregate, vector, struct, function, and selected-authority
  paths supported through explicit local boundaries where appropriate.
- Do not weaken verifier behavior, tests, or expected output.
- Do not retry a globally deprecated `const char*` constructor: it produces
  broad warning noise despite source-compatible compilation.
- The parsed typed-call boundary intentionally covers text supplied by the
  parser and its legacy string-mirror fallback; do not treat it as evidence
  that the generic `LirTypeRef` constructors should be deprecated.

## Proof

- `cmake --build --preset default` succeeded and emitted only the scoped
  `parsed_typed_call_argument_text` deprecation warnings for the two selected
  parsed/re-owned typed-call sites.
- `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  passed; `test_after.log` is the focused proof log.
