Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit And Demote Remaining Template Substitution String Fallbacks

# Current Packet

## Just Finished

Completed Step 5 fallback audit for the covered owner/member typedef and NTTP
substitution surfaces. Classified the remaining rendered-name fallbacks as:
legacy owner-name bridges for records without structured owner identity,
legacy member-name bridges for records with no `member_typedef_text_ids` array,
and NTTP/type parameter spelling bridges only when no parameter TextId metadata
is present. Demoted the member typedef helpers in `types/base.cpp` and
`types/declarator.cpp` so metadata-bearing `member_typedef_text_ids` arrays no
longer fall back to rendered member names when the indexed structured metadata
is missing.

## Suggested Next

Next packet target: supervisor should decide whether Step 5 closes the active
runbook or whether a reviewer/plan-owner pass is needed before lifecycle
closure.

## Watchouts

- Do not weaken tests or add named-fixture shortcuts to make a narrow case pass.
- Member typedef rendered-name fallback now remains only for records without a
  `member_typedef_text_ids` array; metadata-bearing records with missing member
  slots produce a structured miss.
- Existing owner-name and NTTP spelling fallbacks were left in place where they
  are guarded as no-metadata compatibility bridges rather than post-miss
  overrides.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_member_typedef_binding_structured_metadata|cpp_hir_member_typedef_origin_binding_structured_metadata|cpp_hir_member_typedef_realized_owner_structured_metadata|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_alias_member_substitution_structured_metadata|cpp_hir_parser_type_base_member_substitution_structured_metadata)$' --output-on-failure > test_after.log 2>&1`

Full build and focused ctest output captured in `test_after.log`.
