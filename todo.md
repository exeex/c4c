# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's alias-template member-typedef carrier repair is implemented without
reintroducing `TypeSpec::tag`. Parser alias-template references now project the
structured `ParserAliasTemplateInfo::member_typedef` /
`ParserAliasTemplateMemberTypedefInfo` payload onto the reference expression's
`TypeSpec`, including the owner template key, concrete owner arguments, and the
deferred member typedef text id for
`using add_lvalue_reference_t = typename add_lvalue_reference<T>::type`.

HIR template-global instantiation now accepts that projected expression type
when a syntactically value-shaped argument is supplied for a type template
parameter. The mangling path preserves the alias owner/argument carrier for
fully concrete projected alias-template member typedefs, while avoiding the
over-broad deferred-member primary lookup that had caused existing generic
member owner chains like `box<T>` and `leaf<T>` to realize before template
parameter substitution.

## Suggested Next

Supervisor should review and commit this Step 6 repair slice, or choose any
additional milestone-level validation needed before lifecycle handoff.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not infer alias-template targets from `_t` spelling, debug text,
  `tag_ctx`, rendered names, or module dump strings. This slice carries the
  alias member typedef through parser-owned structured metadata.
- Origin-carrier mangling must stay gated to concrete template arguments; using
  it for unresolved template parameters recreates fake owners such as
  `box_T_tag_ctx...` and breaks existing member typedef owner chains.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|frontend_)' > test_after.log 2>&1`

Result: passed.

`cmake --build build` passed. The frontend/HIR subset passed 117/117 tests,
including `cpp_hir_template_inherited_member_typedef_trait` and the repaired
regressions `cpp_hir_template_member_owner_chain`,
`cpp_hir_template_member_owner_decl_and_cast`,
`cpp_hir_template_member_owner_field_and_local`, and
`cpp_hir_template_member_owner_signature_local`.

Canonical proof log: `test_after.log`.
