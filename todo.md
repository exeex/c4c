# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Repair the targeted warning-inventory route

## Just Finished

- Step 6 targeted warning-inventory repair complete: added the deprecated,
  searchable `LirTypeRef::stored_extern_declaration_return_text` runtime-text
  compatibility boundary and used it only for the non-struct path in
  `LirModule::extern_return_type_ref`. Stored external declaration return text
  remains dynamic; no builtin enum was guessed and the parsed call boundaries
  remain unchanged.

## Suggested Next

- Step 6: classify the `stmt.cpp` lowering-return type-text constructions as
  the next bounded retained-runtime-text boundary or closed-set enum candidate.

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
- Extern declaration return type text is stored/external payload and may be
  dynamic; retain its local compatibility boundary rather than guessing a
  builtin enum.

## Proof

- `cmake --build --preset default` succeeded. Its deprecation warnings named
  only deliberate local inventory boundaries: parsed typed-call argument text,
  parsed typed-call return text, and stored/re-owned extern-declaration return
  text.
- `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_extern_decl_type_ref$' > test_after.log`
  passed; `test_after.log` is the focused proof log.
