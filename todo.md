# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's focused HIR consteval/layout repair recovered the three record-layout
consteval positive cases without reintroducing `TypeSpec::tag`. Consteval template type
bindings now canonicalize parser-side record identity onto HIR text/namespace
owner identity before pending evaluation, and pending consteval evaluation now
mirrors type bindings through the structured/text binding maps needed by nested
consteval calls.

The temporary debug probes in `src/frontend/hir/impl/templates/global.cpp` were
removed, `src/frontend/sema/consteval.cpp` has no remaining diff, and the
unsolved template-global lookup experiment was dropped from this slice. The
inherited member typedef trait case remains blocked before specialization:
`ns::is_reference_v<ns::add_lvalue_reference_t<int>>` arrives in HIR as a
value-shaped template argument expression for a type template parameter.

## Suggested Next

Project the parser-owned alias-template member-typedef carrier into HIR-visible
metadata, then rerun this Step 6 proof. The needed carrier is the
`ParserAliasTemplateInfo::member_typedef` / `ParserAliasTemplateMemberTypedefInfo`
payload for `using add_lvalue_reference_t = typename add_lvalue_reference<T>::type`
attached either to the alias template reference expression or to a HIR-visible
alias-template registry keyed by structured alias identity.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not infer alias-template targets from `_t` spelling, debug text, or
  `tag_ctx`/rendered strings. The current HIR-visible expression only carries
  alias structured name identity plus the explicit `int` argument; it does not
  carry the alias target `typename add_lvalue_reference<T>::type`.
- The remaining failure should stay blocked until a structured alias-template
  carrier is exposed; weakening the positive HIR contract would be overfit.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_deferred_consteval_incomplete_type|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain)$' > test_after.log 2>&1`

Result: passed.

`cmake --build build` passed. The consteval/layout cases
`cpp_hir_deferred_consteval_incomplete_type`,
`cpp_hir_if_constexpr_branch_unlocks_later`, and
`cpp_hir_multistage_shape_chain` passed.

Remaining known Step 6 broad-gate blocker:
`cpp_hir_template_inherited_member_typedef_trait`, where HIR still emits no
instantiated `ns::is_reference_v_T_Tns::add_lvalue_reference_T_int` global
because the alias-template argument is not materialized as a type.

Canonical proof log: `test_after.log`.
