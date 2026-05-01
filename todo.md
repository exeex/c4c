# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5B.1
Current Step Title: Add Record-Scope Using-Alias Member-Typedef Carrier

## Just Finished

Step 2.4.4.5B tested the direct `typename Owner<Args>::member` carrier route
against the remaining using-alias projector. Deleting
`apply_alias_template_member_typedef_compat_type` still made
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp` hit
`PARSE_TIMEOUT`, so the projector was retained.

The exact missing carrier is record-scope using-alias registration for
`using name = typename Owner<Args>::member;`: timeout samples are in
`try_parse_record_using_member` -> `parse_type_name` ->
`parse_dependent_typename_specifier`, which still has to materialize the RHS as
a `TypeSpec` because no record-member using-alias registration surface carries
the structured owner `QualifiedNameKey`, substituted owner args, and member
`TextId` directly.

## Suggested Next

Next executable packet is Step 2.4.4.5B.1: add the record-scope
using-alias RHS carrier for `using name = typename Owner<Args>::member;`.

Packet target:

- Preserve the owner `QualifiedNameKey`, structured/substitutable owner args,
  and member `TextId` directly at the record-scope using-alias registration
  boundary.
- Route the path sampled in `try_parse_record_using_member` ->
  `parse_type_name` -> `parse_dependent_typename_specifier` through that
  carrier instead of materializing the RHS as a deferred member `TypeSpec`.
- Keep the accepted Step 2.4.4.5B partial deletions: the alias-template
  context fallback and dependent rendered/deferred `TypeSpec` projection in
  `base.cpp` stay deleted.
- Re-attempt deletion of `apply_alias_template_member_typedef_compat_type`
  only after the record-scope carrier covers the two timeout fixtures named
  below.
- If this carrier cannot fit inside parser/Sema ownership for idea 139, stop
  and report the exact ownership boundary before Step 2.4.4.6.

## Watchouts

- `apply_alias_template_member_typedef_compat_type` is still a live blocker.
  Deleting only that using-alias compatibility projector made
  `cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
  `cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp` hit
  `PARSE_TIMEOUT`. Restoring that projector while keeping the two
  `base.cpp` deletions made the same fixtures pass.
- The retained blocker belongs before Step 2.4.4.6: the parser still needs a
  direct structured RHS carrier for `typename Owner<Args>::member` aliases so
  top-level using-alias registration does not need to fabricate a deferred
  member `TypeSpec`.
- The direct top-level alias-template carrier alone is insufficient: the
  timeout fixtures also exercise record-scope `using` member typedef aliases,
  and that registration path is outside this packet's owned files.
- Alias-of-alias dependent member typedefs remained green with the two
  `base.cpp` bridge removals; no named-test shortcut or expectation downgrade
  was added.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. Build succeeded and `cpp_positive_sema_` passed 881/881.
The projector remains present; the proof is for the retained-projector state
after the deletion attempt exposed the missing record-scope carrier.

`test_after.log` is the canonical proof log for Step 2.4.4.5B.
