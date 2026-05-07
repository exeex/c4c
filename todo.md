Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Qualified-Name Authority

# Current Packet

## Just Finished

Step 1: Inventory Qualified-Name Authority completed an inventory of parser,
shared, Sema, and HIR surfaces that accept a single `TextId` whose spelling may
contain `::` and use it for semantic lookup.

Parser inventory:
- `qualified_key_in_context(Parser&, context_id, TextId, create_missing_path)`
  is compatibility authority when the spelling contains `::`: it delegates to
  rendered-spelling split/intern helpers instead of requiring an existing
  structured carrier. For unqualified spellings it is structured semantic
  authority for namespace-context keys.
- `find_compatibility_key_from_rendered_qualified_spelling()` and
  `intern_compatibility_key_from_rendered_qualified_spelling()` are explicit
  compatibility authority: they split one rendered `TextId` spelling on `::`
  into a `QualifiedNameKey`.
- `Parser::has_typedef_type(TextId)` and `Parser::find_typedef_type(TextId)`
  are typedef semantic lookup authority for qualified spellings because they
  probe the compatibility key before falling back to unqualified/non-atom
  tables.
- `Parser::find_visible_typedef_type(TextId)`, `resolve_visible_type(TextId)`,
  `lookup_type_in_context(context_id, TextId, ...)`, and
  `struct_typedef_key_in_context(context_id, TextId)` are type/typedef lookup
  authority wrappers; direct structured `QualifiedNameRef` paths exist, but
  these wrappers still allow qualified rendered spelling to become authority
  through `qualified_key_in_context()` or `find_typedef_type(TextId)`.
- `Parser::find_var_type(TextId)`, `find_visible_var_type(TextId)`,
  `register_var_type_binding(TextId, ...)`, `resolve_visible_value(TextId)`,
  `lookup_value_in_context(context_id, TextId, ...)`,
  `known_fn_name_key_in_context(context_id, TextId)`, and
  `intern_semantic_name_key(TextId)` are value/function lookup or registration
  authority. `register_var_type_binding()` and lookup fallback paths can create
  or probe a structured key from rendered qualified spelling.
- `Parser::register_known_fn_name_in_context(context_id, TextId)` is function
  compatibility registration authority: for qualified rendered spelling it
  registers both the `qualified_key_in_context()` result and the explicit
  compatibility key.
- `Parser::register_concept_name_in_context(context_id, TextId)`,
  `has_structured_concept_name(QualifiedNameKey)`,
  `resolve_visible_concept(TextId)`, `lookup_concept_in_context(context_id,
  TextId, ...)`, and `is_concept_name(TextId)` are concept lookup authority;
  qualified rendered spelling can enter via `known_fn_name_key()` /
  `qualified_key_in_context()`.
- `Parser::find_named_namespace_child(parent_id, TextId)` and
  `ensure_named_namespace_context(parent_id, TextId)` are unqualified namespace
  segment authority, not compound-name authority; structured
  `QualifiedNameRef` namespace resolution walks one segment `TextId` at a time.
- `Parser::peek_qualified_name()`, `parse_qualified_name()` call sites,
  `qualified_name_key(QualifiedNameRef)`, `resolve_qualified_value()`,
  `resolve_qualified_type()`, and `resolve_namespace_*()` are structured
  semantic authority or parse-time grammar disambiguation, not rendered
  `TextId` authority.
- `render_lookup_name_in_context()`, `render_name_in_context()`,
  `qualified_name_text()`, and visible-name spelling helpers are
  diagnostics/display unless their output is later re-interned by one of the
  compatibility surfaces above.

Shared inventory:
- `has_qualified_name_scope(TextId, TextTable&)` is a spelling predicate for
  compatibility/display decisions.
- `split_qualified_name_scope(TextId, TextTable&)` is rendered-string split
  compatibility authority: it interns owner/base `TextId`s from one spelling.
- `qualified_name_base_matches(TextId, TextId, TextTable&)` is semantic-risk
  compatibility because it answers identity questions by suffix/base splitting.
- `strip_text_id_suffix(TextId, suffix, TextTable&)` is spelling
  compatibility/display support, not qualified-name lookup by itself.
- `NamePathTable`, `QualifiedNameKey`, `QualifiedNameRef`, and
  `QualifiedNameLookupTable` are structured semantic authority when populated
  from token-sequence metadata rather than rendered spelling.

Sema inventory:
- `SemaStructuredNameKey`, `sema_local_name_key()`,
  `sema_structured_name_key()`, `sema_symbol_name_key()`,
  `sema_qualified_symbol_name_key()`,
  `sema_record_owner_key_from_qualifier()`, and
  `sema_using_value_alias_target_key()` are structured semantic authority.
- Record-domain maps in `validate.cpp` keyed by `SemaStructuredNameKey`
  (`struct_defs_by_key_`, complete record sets, field/static-member/base maps)
  are structured semantic authority.
- `structured_record_keys_by_tag_`, legacy string maps such as `funcs_`,
  `consteval_funcs_`, template type-param name maps, and rendered-tag mirrors
  are compatibility or diagnostics/test mirrors, not desired final authority.
- `lookup_struct_static_member_type_by_key()`,
  `resolve_record_owner_key_from_qualifier()`,
  `lookup_struct_static_member_type()`,
  `has_struct_instance_field_by_key()`,
  `resolve_struct_instance_field_text_id_by_name()`, and
  `has_struct_instance_field(TextId, const char*)` are structured record/member
  lookup authority; the single member `TextId` is unqualified member authority.
- `record_template_type_param()`, `is_known_template_type_param()`,
  `is_current_template_type_param()`, and related `*_text_id` maps are
  unqualified template-parameter authority, with legacy string fallback when
  metadata is missing.
- `ConstEvalEnv` text maps, `lookup_type_binding_by_text()` /
  `lookup_type_binding_by_text_id()`, `ConstEvalValueLookupEnv::lookup_by_text()`,
  and `lookup_consteval_function_by_text()` are unqualified `TextId` authority
  only; they explicitly reject qualified AST nodes or are keyed through name to
  unqualified text metadata.
- `lookup_type_binding_by_key()`,
  `lookup_type_binding_by_typespec_key()`,
  `ConstEvalValueLookupEnv::lookup_by_key()`, and
  `lookup_consteval_function_by_key()` are structured/deferred carrier handoff
  authority. Legacy `lookup(const std::string&)` and consteval rendered-name
  maps are compatibility fallback.

HIR inventory:
- `HirRecordOwnerKey`, `NamespaceQualifier`, `make_hir_record_owner_key()`,
  `Module::find_struct_def_tag_by_owner()`, and
  `Module::find_struct_def_by_owner_structured()` are structured semantic
  authority.
- `TypeSpec::tag_text_id`, `template_param_text_id`,
  `template_param_owner_text_id`, `deferred_member_type_text_id`, and
  qualifier `TextId` arrays are structured or deferred carrier fields when
  paired with namespace/owner/index metadata; a lone `tag_text_id` used through
  rendered spelling remains compatibility-risk authority.
- Template/NTTP maps keyed by `TextId` in HIR (`tpl_bindings_by_text`,
  `NttpTextBindings`, `TypeBindingTextMap` consumers,
  `apply_signature_template_binding_by_text()`,
  `apply_lvalue_template_binding_by_text()`,
  `find_template_type_binding_for_call()`, and similar template binding
  helpers) are unqualified template-parameter authority.
- `Lowerer::substitute_signature_template_type()` has two important surfaces:
  the `template_param_text_id` path is unqualified template-parameter authority,
  while `resolve_qualified_member_typedef_by_text_id()` is structured
  qualified member-typedef authority for one-owner-segment carriers.
- `member_typedef_compatibility_name()`,
  `qualified_type_spelling_from_text_ids()`, and the later
  `qualified_name.rfind("::")` fallback in `substitute_signature_template_type()`
  are compatibility authority because they render/split qualified spelling to
  resolve member typedefs.
- `CompileTimeState::find_template_def(std::string)`,
  `find_template_struct_def(std::string)`, `find_consteval_def(std::string)`,
  rendered template specialization maps, and rendered fallback in
  `find_structured_node_entry()` / `find_structured_vector_entry()` are
  compatibility authority.
- `CompileTimeState::find_template_def(Node*)`,
  `find_template_struct_def(Node*)`, `find_template_struct_def(QualifiedNameKey)`,
  `find_template_struct_specializations(Node*)`, and `find_consteval_def(Node*)`
  are structured semantic authority or deferred carrier handoff.
- HIR method/member lookup helpers that accept rendered owner/member strings
  (`resolve_member_lookup_owner_tag()`, `resolve_struct_method_lookup_owner_tag()`,
  `find_struct_method_mangled()`, `find_struct_static_member_*()`,
  `find_struct_member_symbol_id()`) are rendered-string compatibility
  authority when they are reached from a type carrier that could have preserved
  owner/key metadata.

First lookup family to migrate next: parser typedef/type lookup, starting at
`Parser::has_typedef_type(TextId)`, `Parser::find_typedef_type(TextId)`,
`Parser::find_visible_typedef_type(TextId)`, `Parser::resolve_visible_type(TextId)`,
and `Parser::lookup_type_in_context(int, TextId, ...)`, then splitting their
compound-name behavior into structured `QualifiedNameRef` / `QualifiedNameKey`
entry points while keeping `TextId` entry points unqualified-only.

## Suggested Next

Executor should run Step 2 for the parser typedef/type lookup family above:
make `TextId` typedef/type entry points unqualified-only and route compound
names through existing structured `QualifiedNameRef` / `QualifiedNameKey`
surfaces.

## Watchouts

The first migration should avoid broad HIR or consteval cleanup. The narrow
hazard is that parser type disambiguation currently relies on
`find_typedef_type(TextId)` accepting rendered qualified spelling; replacing it
must preserve grammar probes through structured keys instead of disabling the
case. Treat shared helpers `split_qualified_name_scope()` and
`qualified_name_base_matches()` as compatibility support until their parser/HIR
callers are migrated.

## Proof

`printf 'Inventory-only Step 1; no implementation files changed; build/test not run.\n' > test_after.log`

Proof is sufficient for this inventory-only packet. No build/test was run
because no implementation files were changed.
