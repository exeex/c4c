# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.3
Current Step Title: Convert C-Style Cast Type-Id Member-Typedef Consumer

## Just Finished

The deletion probe for the old Step 2.4.4.3 re-audited the remaining rendered
`owner::member` member-typedef mirror after Step 2.4.4.2. The public
`register_struct_member_typedef_binding(owner, member, type)` helper has no
production writer left, but deleting the record-body rendered scoped typedef
writer in `register_record_member_typedef_bindings` exposed a live semantic
consumer.

Concrete blocker: C-style cast/type-id parsing still depends on the rendered
`Box::AliasL` / `Box::AliasR` typedef entries produced from record-body member
typedefs. Removing the `source_tag + "::" + member_name` `register_typedef_binding`
write from `src/frontend/parser/impl/types/struct.cpp` made
`cpp_positive_sema_c_style_cast_member_typedef_ref_alias_basic_cpp` fail with
`cast to unknown type name 'Box::AliasL'` and `Box::AliasR`; related selected
positive cases also failed or timed out. The attempted deletion was reverted, so
no rendered-string workaround was moved or kept as new progress.

## Suggested Next

Step 2.4.4.3 is now the next bounded packet: add a structured C-style
cast/type-id member-typedef consumer for non-template record-body member
typedefs, so `Box::AliasL` and `Box::AliasR` resolve through record/member
metadata instead of the rendered generic typedef table. After that carrier
exists and is proven, proceed to Step 2.4.4.4 to retry deleting the record-body
rendered scoped typedef writer.

## Watchouts

`register_struct_member_typedef_binding(owner, member, type)` remains only
because the record-body rendered mirror still has a semantic consumer. Do not
delete only that helper as progress unless the C-style cast/type-id consumer has
first moved to structured metadata.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Failed during the deletion probe and recorded the blocker in `test_after.log`.
The final code edits were reverted; only this `todo.md` blocker update remains.
