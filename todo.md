# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Plan-owner review accepted Step 1 - Inventory TypeSpec Tag Surfaces as
complete enough to advance. The inventory used `rg` across
`src/frontend/parser`, `src/frontend/sema`, and `src/frontend/hir`; raw text was
enough to separate the current surface families.

Classified surface map:

- `src/frontend/parser/ast.hpp`: `TypeSpec::tag` is declared as the legacy
  rendered spelling bridge for struct/union/enum tags and typedef names.
  `tag_text_id`, template parameter owner/index/TextId metadata, `record_def`,
  qualifier TextIds, namespace context, `tpl_struct_origin_key`,
  `TemplateArgRefList`, and `deferred_member_type_text_id` are the structured
  carriers. `encode_template_arg_debug_ref`, `encode_template_arg_debug_list`,
  and debug-text helpers are display/dump payloads, not semantic authority.
- Parser type/declaration producers in
  `parser/impl/types/base.cpp`, `types/struct.cpp`,
  `types/declarator.cpp`, `declarations.cpp`, `expressions.cpp`, and
  `core.cpp` write `tag` for ordinary named types, synthesized true/false
  types, anonymous/nested record display, constructor/cast spelling,
  template instantiation mangled/final names, member typedef results, and base
  type materialization. Semantic-capable writes usually already sit beside
  `tag_text_id`, `record_def`, `QualifiedNameKey`, `tpl_struct_origin_key`,
  `deferred_member_type_text_id`, or template parameter metadata; generated
  mangled instantiation tags remain final-spelling/materialization payloads.
- Parser semantic reads include typedef-chain lookup, completeness checks,
  type compatibility, template parameter dependency tests, qualified type
  owner recovery, member typedef lookup, and selected-specialization/base
  materialization. Several have structured alternatives (`record_def`,
  `tag_text_id`, member typedef keys, origin keys); raw tag fallback is
  compatibility when no metadata exists. Diagnostic/incomplete-type messages,
  generated names, and cast/constructor variable spelling are display/final
  spelling boundaries.
- Sema `type_utils.cpp::type_binding_values_equivalent` still compares
  `TypeSpec::tag` and qualifier rendered strings as semantic equality even
  though related carriers exist. The same function already prefers
  `tpl_struct_origin_key` over `tpl_struct_origin` and
  `deferred_member_type_text_id` over `deferred_member_type_name`, making this
  the cleanest first migration target.
- Sema `consteval.cpp::resolve_type` and record layout lookup already use
  template-param key/TextId carriers and `record_def`/`tag_text_id` owner keys
  before rendered fallbacks. Rendered `ts.tag` there is explicit no-metadata
  compatibility for type binding names and `ConstEvalEnv::struct_defs`.
- Sema `validate.cpp` uses `record_def` and `tag_text_id`/namespace metadata
  for structured record keys before rendered tag fallbacks; `this_ts.tag` and
  rendered complete-struct maps are compatibility/display boundaries until the
  broader parser/Sema record maps stop being rendered-primary.
- Sema `canonical_symbol.cpp` writes `TypeSpec::tag` to canonical user spelling
  for nominal types. That is canonical/display spelling storage today; it
  should not be the first semantic migration unless a downstream consumer needs
  canonical symbol identity instead of parser TextIds.
- HIR `hir_ir.hpp` defines the structured record carrier:
  `HirRecordOwnerKey`, `make_hir_record_owner_key`,
  `make_hir_template_record_owner_key`, `struct_def_owner_index`, and
  `find_struct_def_by_owner_structured`. `Module::struct_defs` remains a
  rendered-tag compatibility/codegen/dump storage map.
- HIR layout consumers in `hir_types.cpp`, `impl/expr/builtin.cpp`, and Sema
  consteval bridges already prefer `record_def` or
  `tag_text_id`/namespace-derived `HirRecordOwnerKey` before `ts.tag`.
  Residual rendered `struct_defs.find(ts.tag)` calls are compatibility when
  metadata is missing.
- HIR member/static/member-typedef routes in `hir_types.cpp`,
  `hir_functions.cpp`, `impl/templates/type_resolution.cpp`,
  `impl/templates/global.cpp`, `impl/templates/struct_instantiation.cpp`,
  `impl/stmt/decl.cpp`, `impl/stmt/stmt.cpp`, `impl/expr/object.cpp`, and
  `impl/expr/scalar_control.cpp` mix structured `record_def`/owner-key paths
  with rendered `tag + "::member"` maps and `resolve_struct_member_typedef_type`
  signatures. These are semantic identity surfaces with partial carriers; they
  should be migrated after Sema equality or when a packet focuses on member
  owner keys.
- HIR compile-time/debug payloads in `compile_time_engine.hpp` encode
  `tpl_struct_origin`, `deferred_member_type_name`, and `tag` for pending type
  keys/debug text. Treat them as compatibility/debug payloads until a dedicated
  compile-time type-ref carrier exists.

First executable migration target:

Migrate `src/frontend/sema/type_utils.cpp::type_binding_values_equivalent` to
compare `TypeSpec` identity through structured carriers before rendered
`tag`/qualifier strings. Candidate order: `record_def` identity when both sides
have it; namespace context plus `tag_text_id` and qualifier TextIds when
available; template parameter owner/index/TextId for type params; existing
`tpl_struct_origin_key` and `deferred_member_type_text_id`; then retain rendered
`tag`/qualifier/deferred-name fallback only as explicit no-metadata
compatibility. Add focused coverage where two type bindings have stale or
drifted rendered tags but matching structured metadata.

## Suggested Next

Start Step 2 - Migrate Parser-Owned Semantic Producers with the
`type_binding_values_equivalent` migration in Sema as the first bounded code
packet. Compare `TypeSpec` identity through structured carriers before rendered
`tag`/qualifier strings: `record_def` identity when both sides have it,
namespace context plus `tag_text_id` and qualifier TextIds when available,
template parameter owner/index/TextId for type params, existing
`tpl_struct_origin_key`, and `deferred_member_type_text_id`. Retain rendered
`tag`/qualifier/deferred-name fallback only as explicit no-metadata
compatibility. Add focused stale-rendered-type-spelling coverage for the
migrated equality route. Suggested proof subset:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 2; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Inventory-only packet; no build required by the delegated proof. `rg` was used
for classification and c4c-clang-tools was not needed.

Lifecycle-only Step 1 review advanced the current pointer to Step 2; no build
was required for this plan-owner transition.

`git diff --check -- todo.md` passed.
