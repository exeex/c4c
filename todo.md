# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the parser-owned `cache_legacy_var_type_binding` rendered
spelling route. `register_var_type_binding` now records direct TextId-backed
value types plus structured `QualifiedNameKey` value bindings without
refreshing the legacy symbol cache. `find_var_type(TextId)` now prefers the
TextId table and only preserves explicit legacy symbol-table compatibility as a
last-resort reader.

The focused parser test now seeds a stale legacy symbol-cache value for an
ID-first value registration and proves TextId storage wins while registration
does not rewrite the legacy cache.

## Suggested Next

Execute one more focused Step 2.3 parser-owned cleanup packet against either
the visible typedef fallback depth/guard path or a visible-name
`compatibility_spelling` / `compatibility_name` path where existing
`VisibleNameResult`, `TextId`, or `QualifiedNameKey` metadata can carry the
semantic answer without adding rendered rediscovery helpers.

## Watchouts

- Do not restore string/string_view parser state setter authority. The setters
  are now `TextId`-based; display strings passed to them are fallback/display
  payloads only.
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

## Proof

Passed:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure) > test_after.log 2>&1`

Proof log: regenerated canonical `test_after.log` after this
`cache_legacy_var_type_binding` removal packet (926 tests passed).
