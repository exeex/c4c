# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Lifecycle boundary refresh landed after the Step 2.3 parser setter conversion.
`plan.md` now treats parser/Sema-owned `fallback`, `legacy`, and
`compatibility` semantic routes as active cleanup targets when structured
metadata already exists.

HIR-only rendered lookup routes were moved to
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md`, and
`TypeSpec::tag` field deletion / cross-stage fallout was moved to
`ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.

## Suggested Next

Execute a focused Step 2.3 parser-owned cleanup packet for one remaining
`fallback` / `legacy` / `compatibility` semantic route where the caller already
has a real carrier. Good first candidates are:

- `cache_legacy_var_type_binding` and the associated var/type binding readers,
  if they can be collapsed to `TextId`, `QualifiedNameKey`, namespace context,
  or declaration metadata without adding a rendered rediscovery helper.
- Visible typedef fallback depth/guard paths, if the guarded lookup can be
  expressed through existing `TextId` or visible-name metadata.
- Visible-name `compatibility_spelling` / `compatibility_name` paths, if the
  route can keep `VisibleNameResult` or structured lookup output until display
  spelling is explicitly needed.

Start by naming one exact route and proving whether it is removable now or is a
metadata blocker that belongs in idea 140 or 141.

## Watchouts

- Do not restore string/string_view parser state setter authority. The setters
  are now `TextId`-based; display strings passed to them are fallback/display
  payloads only.
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

Proof log: regenerated canonical `test_after.log` after this setter-conversion
packet (926 tests passed).
