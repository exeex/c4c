# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the remaining `UsingValueAlias` rendered compatibility-name
storage and mutation hook. Using-value alias lookup now requires a structured
target `QualifiedNameKey` and validates that key against known function or
structured value bindings; no-key rendered aliases no longer resolve.

The focused parser tests now seed corrupt rendered bridge storage outside the
alias entry and prove both value and type alias lookup ignore that text, while
no-key aliases are rejected.

## Suggested Next

Continue Step 2.3 with a focused review of remaining parser-owned rendered
`compatibility_spelling` paths and remove only those that still act as semantic
lookup authority instead of display/fallback text.

## Watchouts

- Do not restore string/string_view parser state setter authority. The setters
  are now `TextId`-based; display strings passed to them are fallback/display
  payloads only.
- `lookup_type_in_context` still has a direct global-context local visible
  typedef check. That path is TextId/local-table based and is separate from the
  removed recursive visible typedef fallback probe.
- `var_types_by_text_id` is direct parser TextId metadata for
  `register_var_type_binding`, not a rendered-name rediscovery cache. Direct
  structured registrations intentionally do not populate it, so
  string-facing/TextId-facing lookup does not bridge from structured-only
  storage.
- `find_var_type(TextId)` still has an explicit legacy symbol-table
  compatibility fallback for callers/tests that manually seed
  `parser_symbol_tables().var_types`; Step 2.3 removed parser-owned writes to
  that cache for ordinary value registration.
- The qualified `has_typedef` path was deliberately left compatibility-backed
  for qualified names. A metadata-only attempt using qualifier/base `TextId`s
  passed ordinary qualified casts but regressed
  `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  because it found the uninstantiated structured member typedef (`T`) instead
  of the concrete instantiated owner-member typedef (`int`).
- The exact missing producer metadata is a concrete owner-member `TextId` or
  equivalent domain key for instantiated member typedefs such as
  `ns::holder_T_int::type`. Do not pull this into the next Step 2.3 packet
  unless the work is deliberately scoped as parser/Sema metadata only; full
  `TypeSpec::tag` field deletion belongs to idea 141.
- Do not use idea 139 to absorb HIR-only cleanup. Routes such as
  `find_function_by_name_legacy`, `find_global_by_name_legacy`,
  `has_legacy_mangled_entry`, HIR `NttpBindings`, `LegacyRendered`, or HIR
  `fallback_*` recovery belong to idea 140.
- Remaining `parser_text_id_for_token`, `find_parser_text_id`, `parser_text`,
  `token_spelling`, synthetic token production, diagnostics, display/final
  spelling, and compatibility paths are not automatically Step 2 violations;
  classify them by concrete semantic lookup authority before removing them.
- `UsingValueAlias` is now target-key-only. Do not reintroduce test-only alias
  string mutation; corrupt rendered bridge coverage should be represented by
  legacy rendered tables outside the alias entry.
- A direct `frontend_parser_tests` binary run is outside the delegated proof and
  currently still stops later on qualified `TypeSpec` metadata expectations; the
  required C++ parser/sema/negative ctest subset is green.

## Proof

Passed:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure) > test_after.log 2>&1`

Proof log: regenerated canonical `test_after.log` after removing the
using-value alias compatibility-name bridge (926 tests passed).

Additional compile check passed for the edited unit-test source:
`cmake --build build --target frontend_parser_tests`.
