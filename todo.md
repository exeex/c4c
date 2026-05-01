# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step2_4_5b_3_blocked_projector_deletion_review.md
Current Step ID: Step 2.4.4.5B.2
Current Step Title: Implement Top-Level Template Using-Alias RHS Structured Carrier Follow-Up

## Just Finished

Step 2.4.4.5B.2 completed the structured-carrier follow-up needed to delete
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

Next packet should be supervisor/reviewer chosen. The current dirty slice is
ready for acceptance review against
`review/step2_4_5b_followup_acceptance_review.md`.

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
