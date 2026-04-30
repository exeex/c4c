Status: Active
Source Idea Path: ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify HIR Lookup String Authority

# Current Packet

## Just Finished

Completed Plan Step 1 `Classify HIR Lookup String Authority`.

Classification map:

- `parse_scoped_function_name` plan target: no literal C++ symbol exists in the
  current tree. The concrete HIR path is
  `try_parse_qualified_struct_method_name` in
  `src/frontend/hir/hir_lowering_core.cpp`, which splits `fn->name` at the
  final `::` into rendered `struct_tag`, `method_name`, and lookup `key`.
  Available structured carriers are `NamespaceQualifier` from
  `make_ns_qual(fn)`, declaration `TextId` from `make_unqualified_text_id(fn)`,
  and `HirRecordOwnerKey`/`HirStructMethodLookupKey` once the owner record is
  found. Current rendered-name reads are semantic string authority for
  out-of-class method owner and method-key recovery.
- Out-of-class method owner lookup: `attach_out_of_class_struct_method_defs`
  and `lower_non_method_functions_and_globals` call the rendered split helper,
  check `m.struct_defs.count(method_ref->struct_tag)`, and read
  `struct_methods_[method_ref->key]`. Available structured carriers are the
  function node namespace/name text ids, record owner indexes on `Module`
  (`struct_def_owner_index`, `struct_def_owner_tags`), and existing
  `struct_methods_by_owner_` plus link-name/return-type by-owner mirrors.
  Current `fn->name`/`method_ref->key` reads are semantic string authority;
  mangled/link names remain generated spelling.
- Scoped static-member lookup: qualified `NK_VAR` names in
  `impl/expr/scalar_control.cpp` split `n->name` into rendered `struct_tag`
  and `member`. The path tries to recover a structured owner through
  `TypeSpec.record_def`, `tpl_struct_origin`, `find_template_struct_primary`,
  and `resolve_member_lookup_owner_tag`, but final lookups still pass rendered
  tags into `find_struct_static_member_decl` and
  `find_struct_static_member_const_value`. Available carriers are
  `TypeSpec.record_def`, `HirRecordOwnerKey`, member declaration `TextId`, and
  `HirStructMemberLookupKey` with `struct_static_member_*_by_owner_`. Rendered
  reads are mixed: owner recovery is compatibility fallback when structured
  owner is absent, while `final_lookup_struct_tag`/`member` remain semantic
  string authority at the lookup boundary.
- Template struct primary/specialization lookup:
  `find_template_struct_primary(std::string)` reads `template_struct_defs_` by
  rendered name and only records parity against `template_struct_defs_by_owner_`.
  `find_template_struct_specializations(primary_tpl)` prefers
  `ct_state_->find_template_struct_specializations(primary_tpl,
  primary_tpl->name)` but still falls back to
  `template_struct_specializations_[primary_tpl->name]`; registration dual
  writes rendered and by-owner maps. Available carriers are primary
  declaration node identity, `HirRecordOwnerKey` from
  `make_struct_def_node_owner_key`, and compile-time owner-key maps. Rendered
  primary/specialization reads are semantic string authority with parity
  mirrors; `canonical_template_struct_primary` family-name stripping is legacy
  raw input fallback.
- Struct method maps: `find_struct_method_mangled`,
  `find_struct_method_link_name_id`, and `find_struct_method_return_type`
  first attempt `struct_methods_by_owner_`,
  `struct_method_link_name_ids_by_owner_`, and
  `struct_method_ret_types_by_owner_` through
  `make_struct_method_lookup_key(tag, method, const)`, then fall back to
  rendered `struct_methods_`, `struct_method_link_name_ids_`, and
  `struct_method_ret_types_`. Available carriers are `HirRecordOwnerKey`,
  method `TextId`, and `HirStructMethodLookupKey`. Rendered fallback reads are
  explicit compatibility fallback; the helper that builds the owner key still
  matches `module_->struct_def_owner_index` by rendered tag, so that bridge is
  semantic string authority until direct owner-key callers exist.
- Static-member maps: `find_struct_static_member_decl` and
  `find_struct_static_member_const_value` first attempt
  `struct_static_member_decls_by_owner_` /
  `struct_static_member_const_values_by_owner_`, then fall back to
  `struct_static_member_decls_[tag][member]` and
  `struct_static_member_const_values_[tag][member]`. Available carriers are
  `HirRecordOwnerKey`, member `TextId`, and `HirStructMemberLookupKey`.
  Rendered fallback reads are compatibility fallback; `make_struct_member_lookup_key`
  still uses rendered tag to rediscover owner identity, so that bridge remains
  semantic string authority.
- Member-symbol maps: fields intern rendered `tag + "::" + member` into
  `module_->member_symbols` while dual-writing
  `struct_member_symbol_ids_by_owner_`. `find_struct_member_symbol_id` still
  reads only `module_->member_symbols.find(tag + "::" + member)` and records
  parity. Available carriers are `HirRecordOwnerKey`, field/member `TextId`,
  `HirStructMemberLookupKey`, and `struct_member_symbol_ids_by_owner_`.
  Rendered lookup is semantic string authority; the by-owner map is currently a
  parity mirror rather than lookup authority.

## Suggested Next

First bounded implementation packet: Plan Step 2 should make
`try_parse_qualified_struct_method_name`/out-of-class method attachment
structured-primary. Add a helper that resolves the owner record from the
function node's namespace qualifier and declaration/member text ids, then make
`attach_out_of_class_struct_method_defs` and
`lower_non_method_functions_and_globals` consult `struct_methods_by_owner_`
before using the rendered `struct_tag::method` compatibility key.

## Watchouts

Keep HIR lookup changes structured-primary. Do not extend rendered-name
fallbacks or add testcase-shaped special cases as progress. The plan names
`parse_scoped_function_name`, but the current HIR symbol is
`try_parse_qualified_struct_method_name`; no implementation file should be
renamed as part of the first packet unless the supervisor explicitly scopes
that cleanup.

## Proof

Classification-only proof. Ran targeted `c4c-clang-tool-ccdb` definition and
symbol queries for HIR template/member lookup helpers plus targeted `rg` and
line-range reads; no build/test required and no proof log was produced.
