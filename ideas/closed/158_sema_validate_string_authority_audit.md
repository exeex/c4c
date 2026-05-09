# Sema Validate String Authority Audit

Status: Closed
Created: 2026-05-09
Closed: 2026-05-09

Parent Ideas:
- `ideas/closed/153_parser_sema_record_tag_compatibility_table_retirement.md`
- `ideas/closed/154_parser_sema_qualified_name_text_reparse_retirement.md`
- `ideas/closed/155_template_scope_owner_and_member_substitution_domain_keys.md`
- `ideas/closed/156_parser_support_constexpr_type_helper_domain_tables.md`
- `ideas/closed/157_deferred_syntax_text_payload_contract.md`

## Goal

Audit and begin retiring string-keyed semantic authority inside
`src/frontend/sema/validate.cpp`.

Parser now preserves far more structured carriers: `TextId`s, qualified-name
segments, record-domain keys, template owner/parameter metadata, and explicit
syntax payload boundaries. Sema validate should consume those carriers as its
ordinary semantic path. Rendered `std::string` maps may remain as compatibility,
diagnostics, display mirrors, or legacy no-metadata fallbacks, but they should
not override complete structured metadata.

## Why This Idea Exists

The parser cleanup queue closed ideas 153-157. That makes Sema the next layer
to review. `validate.cpp` already has many structured mirrors beside rendered
maps, but the ordinary state still contains several string-keyed semantic
tables:

- `globals_`
- `enum_consts_`
- `funcs_`
- `consteval_funcs_`
- `ref_overload_sigs_` and `cpp_overload_sigs_`
- `complete_structs_` and `complete_unions_`
- `structured_record_keys_by_tag_`
- `struct_defs_by_unqualified_name_`
- `struct_field_text_ids_by_name_by_key_`
- `template_type_params_`
- `template_type_param_text_id_by_name_`
- local `scopes_` and `structured_scope_names_`

Some are compatibility mirrors. Some are still ordinary semantic lookup. This
idea creates the Sema-side audit boundary and starts converting covered paths
to structured lookup precedence.

## Responsibility Split

Parser owns partial syntax carriers:

- source spelling `TextId`
- qualifier and owner `TextId` sequences
- record/template/value carriers
- syntax/debug/display strings where they are not semantic keys

Sema validate owns eager semantic interpretation:

- globals, functions, enum constants, and local scopes
- record completion and record-domain lookup
- overload set validation
- template type parameter visibility
- consteval function visibility and call environment handoff

HIR may consume validated structured carriers later, but Sema should not
discard parser-provided structured identity and rediscover meaning from
rendered names.

## In Scope

- Inventory string-keyed tables in `src/frontend/sema/validate.cpp` and
  classify each as semantic authority, compatibility mirror, diagnostic/display,
  local syntax name, or unresolved bridge.
- Make global/function/enum/consteval lookup prefer
  `SemaStructuredNameKey`, `TextId`, or domain-specific maps when the reference
  carries complete metadata.
- Ensure complete structured metadata misses fail closed instead of falling
  back to rendered string maps.
- Audit record completion and record lookup tables so
  `complete_*_by_key_` and `struct_defs_by_key_` are authoritative for covered
  paths, while `complete_structs_`, `complete_unions_`, and
  `structured_record_keys_by_tag_` remain compatibility mirrors.
- Audit template type parameter and local scope visibility so structured
  `TextId`/domain metadata wins over rendered names where available.
- Audit consteval call handoff from validate into consteval/HIR binding
  environments without expanding this idea into a full consteval rewrite.
- Add focused tests where stale rendered names or same-spelling names across
  domains must not override structured Sema identity.
- Record any remaining string-map compatibility path with owner, limitation,
  and removal condition.

## Out Of Scope

- Full `src/frontend/sema/consteval.cpp` domain-table conversion.
- Full `src/frontend/sema/canonical_symbol.cpp` template canonicalization
  cleanup.
- HIR `TypeBindings` / `NttpBindings` storage migration.
- Removing diagnostic messages, source spelling, or local debug names.
- Rewriting all overload resolution machinery in one packet.
- Weakening tests or marking supported paths unsupported.

## Candidate Anchors

- `globals_`, `structured_globals_`, and `structured_global_keys_by_name_`
  around global binding and lookup.
- `funcs_`, `structured_funcs_`, `structured_function_names_`, and overload
  maps around function declaration conflict checks.
- `consteval_funcs_`, `consteval_funcs_by_text_`, and
  `consteval_funcs_by_key_` around consteval function lookup.
- `complete_structs_`, `complete_unions_`, `complete_structs_by_key_`,
  `complete_unions_by_key_`, `struct_defs_by_key_`, and
  `structured_record_keys_by_tag_` around record completion.
- `template_type_params_`, `template_type_param_text_ids_`, and
  `template_type_param_text_id_by_name_` around template type parameter
  visibility.
- `scopes_`, `structured_scope_names_`, and `structured_scopes_` around local
  bindings.
- The consteval call binding handoff that still builds string NTTP maps while
  also carrying TextId/structured maps.

## Acceptance Criteria

- `validate.cpp` has a documented inventory of remaining string-keyed tables
  and their roles.
- Covered Sema lookup paths use structured keys or `TextId` maps before
  rendered names when complete metadata is present.
- Complete structured metadata misses do not reopen rendered string fallback
  for covered paths.
- Retained rendered-name maps are explicitly classified as compatibility,
  diagnostics/display, or no-metadata fallback with removal conditions.
- Focused tests prove stale rendered names cannot override structured Sema
  identity in at least one global/function/record/template or local-scope path.

## Completion Notes

The active runbook completed all six steps. `validate.cpp` now documents the
remaining string-keyed Sema validate paths as semantic authority with
structured mirrors, compatibility mirrors, diagnostics/display, local syntax
names, or no-metadata fallbacks, including removal conditions for retained
rendered-name bridges.

Covered value/function, record, template/local, and consteval handoff paths now
prefer structured identity where complete metadata is available, and focused
tests cover stale rendered-name rejection for the converted lookup families.
Milestone validation passed the full suite with 3024/3024 tests passing, and
the close-time regression guard passed against the matching full-suite
baseline.

## Reviewer Reject Signals

- A slice claims Sema string authority is retired while ordinary lookup still
  runs through rendered `std::string` maps first.
- A change treats raw `TextId` alone as cross-domain semantic identity without
  record/function/value/template domain context.
- Tests only update diagnostics or rendered spelling expectations.
- The route expands into HIR or full consteval rewrite before the validate
  authority audit is complete.
- A supported path is weakened or marked unsupported to avoid structured lookup
  failures.
