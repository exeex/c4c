# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the dependent-typename visible-type rendered-spelling
re-entry route in `src/frontend/parser/impl/types/declarator.cpp`.

`parse_dependent_typename_specifier` no longer assigns `resolved` from
`visible_name_spelling(resolve_visible_type(...))` and no longer gates its
fallback path with `has_typedef_type(find_parser_text_id(resolved))`. The
remaining success authority is the direct local visible typedef payload,
structured qualified typedef metadata, or the existing structured owner/member
probe. `frontend_parser_lookup_authority_tests` now covers a `typename Alias`
case where visible-type rendering produces `StructuredNs::Target` and a legacy
full-rendered typedef exists, but no direct typedef authority exists for the
alias name; parsing preserves `Alias` and reports no structured type payload.

## Suggested Next

Request reviewer or plan-owner routing for Step 2.3 exhaustion unless the
supervisor has another concrete parser/Sema rendered-spelling authority route.

## Watchouts

- The reviewer finding was validation sufficiency only: the edited white-box
  assertions were not executable through the delegated green CTest subset. The
  new target addresses that without adding a monolithic-binary switch.
- Declaration-side qualified-name classification still intentionally keeps
  unresolved qualified heads on the type side unless structured value lookup
  proves an expression. The removed route was only the semantic lookup success
  result for missing unqualified global `::name`.
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
- The known-function value route is now key-authoritative for lookup success,
  but it still fills `compatibility_spelling` opportunistically as display
  payload. Do not convert that display field into a new semantic wrapper.
- `find_alias_template_info_in_context` is now key-authoritative only. If a
  future alias-template caller needs a using-alias target, it needs structured
  metadata for that target rather than a rendered-spelling bridge.
- `resolve_qualified_typedef_name` is now key/result-authoritative for qualified
  typedef success. Do not reintroduce a full rendered-name `TextId` bridge to
  prove qualified typedef presence; if a qualified member typedef still needs
  instantiated owner/member metadata, keep that as a separate metadata packet.
- The dependent-typename declarator route now deliberately ignores legacy
  full-rendered typedef storage when the only lead came from visible-type
  display rendering. Do not restore that bridge through another helper wrapper.
- A direct `frontend_parser_tests` binary run is outside the delegated proof and
  currently still stops on qualified `TypeSpec` metadata expectations
  (`qualified TypeSpec should carry the base-name TextId metadata`); the
  required C++ parser/sema/negative ctest subset is green.

## Proof

Passed:
`(cmake --build build --target frontend_parser_lookup_authority_tests && ctest --test-dir build -R '^frontend_parser_lookup_authority_tests$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (1 focused CTest passed, including the drifted
qualified typedef and dependent-typename visible-type rendered-spelling
re-entry coverage).
