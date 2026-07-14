# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 6 targeted warning-inventory repair complete: added the deprecated,
  searchable `LirTypeRef::parsed_typed_call_return_text` compatibility
  boundary and used it only when `make_lir_call_op` re-owns parsed typed-call
  return text. The existing parsed argument boundary remains unchanged; both
  local boundaries retain dynamic aggregate, vector, struct, and function
  spellings as runtime-text compatibility paths.

## Suggested Next

- Step 6: inspect the remaining warning inventory and select the next bounded
  closed-set migration or retained runtime-text compatibility boundary.

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
- The parsed typed-call return boundary is intentionally limited to
  `make_lir_call_op`; do not broaden it to unrelated return-type construction.

## Proof

- `cmake --build --preset default` succeeded. Its only deprecation warning
  names were the deliberate local `parsed_typed_call_argument_text` and
  `parsed_typed_call_return_text` inventory boundaries.
- `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  passed; `test_after.log` is the focused proof log.
