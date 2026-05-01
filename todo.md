# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step2_4_5b_1_failed_record_scope_carrier_review.md
Current Step ID: Step 2.4.4.5B.2
Current Step Title: Implement Reviewed Record-Scope Using-Alias RHS Sidecar Carrier

## Just Finished

Step 2.4.4.5B.2 is implemented as the reviewed record-scope using-alias RHS
sidecar carrier.

Completed work:

- `try_parse_record_using_member` now tentatively captures
  `using name = typename Owner<Args>::member;` RHS metadata before
  `parse_type_name()`.
- `ParserRecordBodyState` carries a parallel
  `ParserAliasTemplateMemberTypedefInfo` sidecar for record member aliases.
- Parser template state persists valid record member typedef sidecars by
  structured `Record::alias` `QualifiedNameKey`.
- Record member typedef lookup now tries the sidecar before the flattened
  `TypeSpec`, substitutes record template arguments into RHS owner args, and
  resolves through structured template-instantiation or selected
  primary/specialization member typedef lookup.
- The two timeout fixtures pass under the delegated `cpp_positive_sema_` proof.

## Suggested Next

Next packet should retry Step 2.4.4.5B.3: delete
`apply_alias_template_member_typedef_compat_type` and prove the same
`cpp_positive_sema_` subset, with explicit confirmation that
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp` still pass.

## Watchouts

- `apply_alias_template_member_typedef_compat_type` is still present; this
  packet made it ready for the immediate deletion retry rather than deleting it.
- The sidecar route did not parse rendered/debug strings, split owner/member
  spelling, downgrade expectations, or add named-fixture branches.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh build plus 881/881 passing
`cpp_positive_sema_` tests. The two timeout fixtures passed:
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`.
