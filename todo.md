# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5B.1
Current Step Title: Review Failed Record-Scope Carrier Route

## Just Finished

Step 2.4.4.5B.1 attempted a record-scope carrier for
`using name = typename Owner<Args>::member;`, but the route was insufficient.
The implementation changes were reverted, so no implementation progress is
claimed from that attempt.

`apply_alias_template_member_typedef_compat_type` remains retained. Deleting it
still makes `cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp`
and `cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp` hit
`PARSE_TIMEOUT`.

Accepted partial Step 2.4.4.5B results remain in force: the alias-template
context fallback and dependent rendered/deferred `TypeSpec` projection in
`base.cpp` stay deleted.

## Suggested Next

Next packet is review-only Step 2.4.4.5B.1: review the failed record-scope
carrier route before more implementation.

Packet target:

- Trace why `try_parse_record_using_member` -> `parse_type_name` ->
  `parse_dependent_typename_specifier` still reaches
  `apply_alias_template_member_typedef_compat_type` for the two timeout
  fixtures.
- Decide whether the next implementation packet is Step 2.4.4.5B.2
  record-scope using-alias RHS metadata, Step 2.4.4.5C dependent/template
  record member-typedef availability, or another exact parser/Sema-owned
  carrier.
- Name the precise carrier, owned implementation surfaces, and proof command
  before code changes resume.
- Reject another attempt that seeds the carrier from rendered/deferred
  `TypeSpec` fields, `tpl_struct_origin`, `deferred_member_type_name`,
  `qualified_name_from_text`, `qualified_alias_name`, `debug_text`, or split
  rendered `Owner::member` spelling.

## Watchouts

- The reverted record-scope carrier attempt did not cover the two timeout
  fixtures and should not be treated as accepted progress.
- `cpp_positive_sema_` is green only with
  `apply_alias_template_member_typedef_compat_type` retained.
- Do not continue implementation until the review names one exact next
  carrier. "Try the record-scope carrier again" is not precise enough.
- The accepted Step 2.4.4.5B `base.cpp` deletions should stay deleted unless a
  reviewer explicitly identifies a regression that requires a lifecycle route
  reset.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. Build succeeded and `cpp_positive_sema_` passed 881/881.
The projector remains present. The green proof is for the retained-projector
state only.

`test_after.log` is the canonical proof log for Step 2.4.4.5B.1.
