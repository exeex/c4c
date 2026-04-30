# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.1
Current Step Title: Re-Audit Member-Typedef Mirror Writers And Readers

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

The attempted Step 2.4.4 partial mirror route was rejected by
`review/step2_4_4_partial_mirror_review.md`: deleting
`register_struct_member_typedef_binding(owner, member, type)` while rebuilding
the same owner/member key by splitting rendered `mangled` text in
`src/frontend/parser/impl/types/base.cpp` is route drift, and the record-body
writer still leaves semantic rendered scoped typedef storage live.

## Suggested Next

Next bounded packet: execute Step 2.4.4.1 as lifecycle or code-reading
inventory only. Re-audit the member-typedef mirror writers and readers after
Step 2.4.3, especially the record-body writer and the template-instantiation
writer in `src/frontend/parser/impl/types/base.cpp`, then record the smallest
safe next structural conversion or metadata blocker.

The next accepted implementation packet must either pass an existing
structured owner/member carrier into the template-instantiation writer, convert
one still-live semantic reader/writer to structured metadata, or stop on a
missing metadata blocker. It must not delete the wrapper/API while rebuilding
semantic identity from rendered scoped text in a narrower local block.

## Watchouts

The new reader helper intentionally uses existing qualifier/base `TextId`s and
`NamePathTable::find`; it does not split rendered qualified spelling or create
new path authority during lookup. Global-qualified record/member typedef
spellings may need an explicit canonical-key decision in a later packet because
the current direct writer stores non-global canonical qualified paths.

Do not remove the rendered compatibility mirror yet. Do not count helper/API
deletion as progress if semantic rendered scoped typedef storage remains live.
Do not move rendered key construction into `types/base.cpp` by splitting
`mangled` or `owner::member` text; that is the rejected partial mirror route.

The record-body writer in `src/frontend/parser/impl/types/struct.cpp` and the
template-instantiation writer in `src/frontend/parser/impl/types/base.cpp` must
be classified before Step 2.4.4.3 can shrink or delete the mirror.

## Proof

No new code proof for this lifecycle reset. The previous Step 2.4.3 delegated
proof remains recorded in `test_after.log`:

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: 100% tests passed, 0 failed out of 928. The rejected Step 2.4.4 partial
mirror slice is not acceptance proof for the mirror deletion route.
