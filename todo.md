# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.2
Current Step Title: Prove Direct Record Member-Typedef Lookup

## Just Finished

Step 2.4.2 converted the record-body member-typedef writer route for
non-template finalized records. The `using Alias = ...;` and `typedef ...`
record-body parse sites no longer call
`register_struct_member_typedef_binding(current_struct_tag_text(), member,
type)`. Finalization now copies the member typedef arrays to the record `Node`,
uses `namespace_context_id` + `unqualified_text_id` + member `TextId` to build
a direct record/member `QualifiedNameKey`, and registers that structured typedef
binding.

The rendered compatibility mirror is intentionally preserved from finalization
for remaining readers and template-primary behavior. Template-instantiation
registration in `src/frontend/parser/impl/types/base.cpp` was left unchanged.
A focused parser test now proves both record-body writer forms produce direct
record/member keys and that stale rendered storage does not override those
direct keys.

## Suggested Next

Bounded next packet: convert one remaining reader that still enters member
typedef lookup with rendered qualified text to a structured carrier
(`QualifiedNameKey`, `TypeSpec::record_def`, or owner `Node`) without removing
the compatibility mirror. Good candidates remain the `find_typedef_type(TextId)`
qualified-spelling branch or the current-record sibling fallback, but the
supervisor should pick the next reader based on source-idea priority.

## Watchouts

Do not remove the rendered compatibility mirror yet: the delegated proof showed
template-primary alias behavior still depends on remaining compatibility
readers while the template-instantiation writer is out of scope. The direct
record/member registration is deliberately limited to non-template finalized
records; template member typedef semantics should be handled in a dedicated
template-instantiation packet.

Do not add a helper that accepts rendered `owner::member`, `std::string`, or
`std::string_view` qualified text and splits or reparses it into owner/member
identity. This packet added only a key builder from namespace context,
record `TextId`, and member `TextId`.

## Proof

Fresh delegated proof passed and is recorded in `test_after.log`:

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: 100% tests passed, 0 failed out of 928.
