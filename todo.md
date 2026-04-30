# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.3
Current Step Title: Prove Qualified Key Or Namespace-Context Lookup

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

Bounded Step 2.4.3 executor packet: convert one qualified-key or
namespace-context member-typedef reader, preferably `lookup_type_in_context`
or a nearby qualified-type probe path that already has `context_id` plus
member `TextId` / `QualifiedNameKey`, so it reads direct structured typedef
authority without rendering `owner::member` text or reparsing qualified
spelling.

Keep `find_typedef_type(TextId)`'s qualified-spelling branch and the
current-record sibling fallback as later/removal candidates unless the executor
first identifies an upstream caller that can pass an existing
`QualifiedNameKey`, namespace context, or parser qualified-name carrier. Do not
count a local rewrite of those TextId-only boundaries as Step 2.4.3 progress if
it still starts from rendered qualified text.

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

For Step 2.4.3, reject any route that merely moves the rendered qualified-text
lookup behind a new wrapper. The converted reader should start from an existing
structured carrier supplied by the caller: `QualifiedNameKey`,
`QualifiedNameRef`, namespace context plus `TextId`, or record/tag metadata.

## Proof

Fresh delegated proof passed and is recorded in `test_after.log`:

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: 100% tests passed, 0 failed out of 928.
