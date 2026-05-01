# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5B
Current Step Title: Replace The Dependent/Template Member-Typedef Bridge

## Just Finished

Step 2.4.4.5B retried bridge deletion after the alias-of-alias dependent
carrier landed. The alias-template application path now resolves only through
the structured `QualifiedNameKey` lookup and no longer falls back through
`find_alias_template_info_in_context(current_namespace_context_id(),
last_resolved_typedef_text_id)`.

The dependent alias-template member branch also no longer reprojects
structured owner/member metadata into rendered/deferred `TypeSpec` fields when
substituted owner args remain dependent. That branch now only publishes the
substituted structured carrier through `last_using_alias_member_typedef`.

## Suggested Next

Next coherent packet is to add the missing non-`TypeSpec` alias-template RHS
carrier needed to delete `apply_alias_template_member_typedef_compat_type`, or
to split that carrier into a follow-up metadata idea if the supervisor wants
Step 2.4.4.5B parked as partially complete.

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
- Alias-of-alias dependent member typedefs remained green with the two
  `base.cpp` bridge removals; no named-test shortcut or expectation downgrade
  was added.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. Build succeeded and `cpp_positive_sema_` passed 881/881.

`test_after.log` is the canonical proof log for Step 2.4.4.5B.
