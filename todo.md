Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Rendered Registry Families

# Current Packet

## Just Finished

Completed plan Step 1 inventory for rendered HIR registry families before any
implementation or test edits.

### Registry Inventory

Module registries:

- `Module::struct_defs`: rendered-tag storage for `HirStructDef` plus
  deterministic emission/display through `struct_def_order`. Key users include
  AST-to-HIR writes in `hir_types.cpp`/template instantiation, HIR inspection,
  backend layout emission, const-init, field/member layout, and legacy
  compatibility fallback helpers. Structured replacement is partial:
  `struct_def_owner_index` can find a rendered tag from complete
  `HirRecordOwnerKey`, but `struct_defs` still owns the actual definition
  payload and order. Retain as display/order/payload storage; narrow semantic
  lookups to owner-key APIs and fail closed for complete owner-key misses.
- `Module::struct_def_owner_index`: structured owner-key index from
  `HirRecordOwnerKey` to rendered tag, written by `index_struct_def_owner` and
  read by frontend and backend structured layout helpers. Classification:
  structured authority, not a rendered mirror. Complete structured replacement:
  yes for owner identity; retained boundary is that it still points into
  `struct_defs` for payload storage. First implementation target should not
  retire this map; use it to fence `struct_defs` fallback.
- `Module::fn_index`: rendered function-name compatibility index, written by
  `index_function_decl` and read by `lookup_function_id`,
  `find_function_by_name_legacy`, app filtering, and legacy call/global
  fallback paths. Structured replacement exists through `fn_structured_index`
  plus `DeclRef::link_name_id`; `resolve_function_decl` already prefers
  LinkNameId/structured and records parity. Retain only as no-metadata
  compatibility/display bridge; fence complete metadata callers from reopening
  rendered fallback after a structured miss.
- `Module::global_index`: rendered global-name compatibility index, written by
  `index_global_decl` and read by `lookup_global_id`,
  `find_global_by_name_legacy`, template-global materialization, app filtering,
  and legacy global fallback paths. Structured replacement exists through
  `global_structured_index`, `DeclRef::global`, and `DeclRef::link_name_id`.
  Retain only as no-metadata compatibility/display bridge; fence complete
  metadata callers from reopening rendered fallback after a structured miss.

`CompileTimeState` registries:

- `CompileTimeState::template_fn_defs_`: rendered template-function definition
  map, written by `register_template_def` and read by string-only
  `has_template_def`, `find_template_def`, `is_consteval_template`, iteration,
  deferred instance seeding, and dumps. Structured replacement exists through
  `template_fn_defs_by_key_` when callers pass declaration identity. Retain as
  no-metadata compatibility and iteration/display storage; first code target is
  to fail closed in `find_structured_node_entry` when a complete declaration key
  is present but misses.
- `CompileTimeState::template_struct_defs_`: rendered primary template-struct
  map, written by `register_template_struct_def` and read by string-only
  helpers, pending-type canonicalization, lowerer cross-checks, and dumps.
  Structured replacement exists through `template_struct_defs_by_key_` and
  `QualifiedNameKey` lookup. Retain as no-metadata compatibility; fence
  declaration-key lookups from rendered fallback on complete structured miss.
- `CompileTimeState::template_struct_specializations_`: rendered primary-name
  to specialization-list map, written by specialization registration and read by
  fallback specialization lookup and dumps. Structured replacement exists
  through `template_struct_specializations_by_owner_key_` when the primary
  definition has complete identity. Retain as no-metadata compatibility and
  insertion-list storage; fence complete primary-owner lookups.
- `CompileTimeState::consteval_fn_defs_`: rendered consteval-function map,
  written by `register_consteval_def` and read by string-only helpers,
  `consteval_fn_defs()` compatibility envs, and dumps. Structured replacements
  exist through `consteval_fn_defs_by_key_`, `consteval_fn_defs_by_text_`, and
  `consteval_fn_defs_by_consteval_key_`. Retain as no-metadata compatibility;
  fence complete declaration/consteval-key lookups and prefer text/key envs.
- `CompileTimeState::enum_consts_`: rendered enum constant value map, written
  by `register_enum_const` and used as consteval compatibility input and dump
  surface. Structured replacement exists for global enum constants through
  `enum_consts_by_key_`; local/block enum authority remains lowerer scope maps.
  Retain as no-metadata compatibility/diagnostic support; do not treat it as
  local enum authority.
- `CompileTimeState::const_int_bindings_`: rendered global const-int map,
  written by `register_const_int_binding` and used as consteval compatibility
  input and dump surface. Structured replacement exists through
  `const_int_bindings_by_key_` for complete global bindings. Retain as
  no-metadata compatibility; fence complete global const-int key misses.

Lowerer registries:

- lowerer `struct_def_nodes_`: rendered tag/name to AST struct definition map,
  written during initial HIR and template instantiation and read by template
  resolution, static-member evaluation, destructor/member-dtor checks, object
  construction, and deferred NTTP helpers. Structured replacement is partial
  through `struct_def_nodes_by_owner_`; several callers still need AST payload
  by generated/realized tag. Retain as compatibility/payload bridge; narrow
  owner-complete callers to `struct_def_nodes_by_owner_` first.
- lowerer `template_struct_defs_`: rendered primary template-struct map, written
  by `register_template_struct_primary` and read by
  `find_template_struct_primary`, template-env construction, and deferred NTTP
  helpers. Structured replacement exists through
  `template_struct_defs_by_owner_` and `ct_state_` keys. Retain as
  no-metadata compatibility; fence complete owner-key misses.
- lowerer `template_global_defs_`: rendered primary template-global map, built
  by `collect_template_global_definitions` and read by
  `find_template_global_primary`/`ensure_template_global_instance`. No complete
  structured replacement is present in lowerer state. Retain as semantic
  authority for this route-local template-global boundary until a structured
  global-template key is introduced.
- lowerer `struct_static_member_decls_`: rendered owner-tag/member map, written
  during struct lowering and template instantiation and read by static-member
  lookup/evaluation. Structured replacement exists through
  `struct_static_member_decls_by_owner_` plus related const-value/member-symbol
  owner maps. Retain as no-metadata compatibility; owner-complete callers
  should prefer structured and fail closed on structured misses.
- lowerer `struct_methods_`: rendered `tag::method` to mangled-name map, written
  during method lowering/instantiation and read by method lookup, range-for,
  operator/call paths, and constructor first-method compatibility. Structured
  replacement exists through `struct_methods_by_owner_` and link-name/return
  structured mirrors. Retain as no-metadata compatibility and legacy inherited
  lookup bridge; fence complete owner/member lookups.
- lowerer constructor/destructor maps: `struct_constructors_` and
  `struct_destructors_` are rendered struct-tag maps written during method
  lowering and read by object construction, declarations, temporary teardown,
  and member destruction. Constructors carry optional `owner_key` per overload
  but the map itself has no complete structured owner-key replacement;
  destructors have none. Retain as semantic route-local storage until
  owner-key-indexed ctor/dtor maps exist.
- lowerer overload maps: `ref_overload_set_` and `ref_overload_mangled_` are
  rendered base-key and AST-node reverse maps written by method/function
  overload registration and read by operator/call lowering and HIR build. No
  complete structured replacement is present. Retain as route-local overload
  resolution storage; future work needs structured overload-set keys rather
  than retirement.

First implementation target: fence `CompileTimeState::find_structured_node_entry`
and `find_structured_vector_entry` so complete declaration-key lookups for
template functions, template structs, template-struct specializations, and
consteval functions do not fall back to rendered maps after a structured miss.
This is the smallest complete family with centralized helpers and existing
structured mirrors.

Recommended proof command: build plus focused HIR lookup/template metadata tests,
for example `cmake --build build --target frontend_hir_lookup_tests
cpp_hir_template_canonical_primary_origin_metadata_test
cpp_hir_template_realize_struct_metadata_test
cpp_hir_sema_consteval_type_utils_metadata_test -j2 && ctest --test-dir build
-R 'frontend_hir_lookup_tests|cpp_hir_template_canonical_primary_origin_metadata_test|cpp_hir_template_realize_struct_metadata_test|cpp_hir_sema_consteval_type_utils_metadata_test'
--output-on-failure`.

## Suggested Next

Implement the first target by narrowing `CompileTimeState` structured helper
fallbacks, then add/adjust focused tests for same-spelled template or consteval
declarations across structured domains.

## Watchouts

- Step 1 inventory is recorded; implementation/test edits may start only in the
  next packet.
- Do not reopen idea 161 template binding internals or idea 162 backend `LinkNameId` transport.
- Preserve display, diagnostics, insertion-order storage, and route-local generated names unless the inventory proves they are unused.
- Do not let complete structured misses silently fall back to rendered maps except through explicit no-metadata compatibility boundaries.
- `Module::struct_defs` and lowerer `struct_def_nodes_` still own payload/order
  bridges, so they should be fenced rather than mechanically deleted.
- Lowerer `template_global_defs_`, ctor/dtor maps, and overload maps do not yet
  have complete structured replacements; retiring them now would be route drift.

## Proof

Inventory-only packet; no build or ctest required. Ran
`git diff --check -- todo.md` after updating `todo.md`.
