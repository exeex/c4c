# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.3
Current Step Title: Prove Qualified Key Or Namespace-Context Lookup

## Just Finished

Step 2.4.3 converted the qualified-type reader route backed by
`Parser::resolve_qualified_type(const QualifiedNameRef&)`. Qualified type
resolution now builds a direct key from the existing `QualifiedNameRef`
segment/TextId carrier and checks structured typedef storage before falling
back to namespace-context lookup, allowing finalized record-body member
typedefs such as `ns::Owner::UsingMember` to resolve through the direct
record/member key without rendering or reparsing `owner::member` spelling.

The compatibility mirror and `find_typedef_type(TextId)` qualified-spelling
branch remain in place. A focused parser assertion extends the Step 2.4.2
record-body member typedef test to prove the reader prefers the direct
record/member key over stale rendered fallback storage.

## Suggested Next

Next bounded packet: convert another reader that already starts from structured
authority, preferably a namespace-context or `QualifiedNameRef` path feeding
`probe_qualified_type` / functional-cast owner checks, and prove it does not
consult rendered qualified typedef storage before direct structured typedef
metadata.

## Watchouts

The new reader helper intentionally uses existing qualifier/base `TextId`s and
`NamePathTable::find`; it does not split rendered qualified spelling or create
new path authority during lookup. Global-qualified record/member typedef
spellings may need an explicit canonical-key decision in a later packet because
the current direct writer stores non-global canonical qualified paths.

Do not remove the rendered compatibility mirror yet. Template-instantiation
member typedef registration in `src/frontend/parser/impl/types/base.cpp`
still uses the compatibility writer and remains out of scope for this packet.

## Proof

Fresh delegated proof passed and is recorded in `test_after.log`:

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: 100% tests passed, 0 failed out of 928.
