# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.4.4.5C
Current Step Title: Add Dependent/Template Record Member-Typedef Carrier

## Just Finished

Step 2.4.4.5B.2 and Step 2.4.4.5B.3 completed the structured-carrier
follow-up and projector deletion needed to remove
`apply_alias_template_member_typedef_compat_type`.

The parser now rejects `typename Owner<Args>::template member<...>` from the
member-typedef sidecar path, consumes record-scope template using-alias RHS
forms through `ParserAliasTemplateMemberTypedefInfo` instead of reparsing them
into deferred rendered `TypeSpec` carriers, and lets alias-template member
carrier resolution follow an alias-template owner before resolving the final
member typedef. The deleted projector and its call site are gone.

Acceptance-review revision: removed the rendered owner fallback in
`src/frontend/parser/impl/types/base.cpp` that rebuilt an owner `TypeSpec` via
`render_name_in_context`, `TypeSpec::tag`, `tpl_struct_origin`, and
`set_template_arg_refs_from_parsed_args`. The two STL iterator fixtures and the
full delegated subset remain green without that fallback.

Broader-check revision: narrowed record-scope structured RHS consumption in
`src/frontend/parser/impl/types/struct.cpp` to alias-template owners and
template owners with registered specialization families. Simple primary
template owners keep normal RHS type parsing, preserving HIR-visible concrete
types for the `cpp_hir_template_member_owner*` fixtures.

## Suggested Next

Execute Step 2.4.4.5C: Add Dependent/Template Record Member-Typedef Carrier.

Target `register_record_member_typedef_bindings()` in
`src/frontend/parser/impl/types/struct.cpp` and the parser route exercised by:

- `cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`
- `cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`
- `cpp_positive_sema_tuple_element_alias_mix_parse_cpp`

The packet should add or thread a parser/Sema-owned structured carrier for
template/dependent record member typedef availability keyed by structured owner
identity plus member `TextId`, then route those fixtures through it before any
rendered `source_tag::member` publication.

## Watchouts

- Record-scope structured consumption is limited to valid
  `typename Owner<Args>::member` sidecars while a template scope is active; RHS
  member-template forms with a following `<...>` continue through normal type
  parsing.
- The alias-owner resolver intentionally handles the STL shape where an alias
  template owner, such as `__iter_traits<...>`, must first resolve through its
  own structured member carrier before the requested final member is looked up.
- The acceptance-review-blocking fallback through rendered owner spelling was
  removed; do not reintroduce that route.
- The record-scope structured shortcut must stay scoped; broadening it back to
  all `typename Owner<Args>::member` aliases loses HIR materialized types for
  simple owner chains.
- Step 2.4.4.5C must not retry deletion by assuming
  `template_instantiation_member_typedefs_by_key` or
  `ParserAliasTemplateInfo::member_typedef` covers the timed-out
  dependent/template record route; the runbook calls both already
  insufficient for this packet.
- Do not replace the record member typedef bridge with a helper that renders,
  splits, reparses, or stores `source_tag::member`, `std::string`,
  `std::string_view`, fallback spelling, or debug text as semantic authority.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus
881/881 passing `cpp_positive_sema_` tests, including
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`.

Focused broader proof also passed:
`ctest --test-dir build -j --output-on-failure -R '^cpp_hir_template_member_owner'`
with 5/5 passing tests.

Suggested Step 2.4.4.5C proof:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_(eastl_slice7_piecewise_ctor_parse_cpp|step3_timeout_probe_baseline_parse_cpp|tuple_element_alias_mix_parse_cpp)' >> test_after.log 2>&1`
