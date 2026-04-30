# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4
Current Step Title: Audit Parser Type, Tag, And Member-Typedef Routes

## Just Finished

Step 2.4 remains active. The accepted checkpoint at commit `e139897a` removed
only the unused
`register_struct_member_typedef_binding(const std::string&, const TypeSpec&)`
overload that split rendered scoped text into owner/member identity. The live
`register_struct_member_typedef_binding(owner, member, type)` rendered
`owner::member` typedef mirror is still reachable and remains the blocker.

The pending Step 2.4 member-typedef conversion was rejected by
`review/step2_4_member_typedef_conversion_review.md` and its worktree diff was
discarded. That rejected route added
`find_structured_member_typedef_type(std::string_view qualified_name)`, parsed
rendered qualified text back into owner/member identity, and then tried to
recover structured record/member lookup from that rendered input. The reviewer
classified that as route drift because it preserves rendered-string semantic
rediscovery behind a new wrapper.

## Suggested Next

Delegate a narrow Step 2.4 follow-up for the remaining member-typedef blocker:
replace the live `owner::member` typedef mirror consumers with lookup that
starts from an existing structured carrier, then delete or shrink the mirror
only after the structured route owns those consumers.

Acceptable starting carriers include owner `TypeSpec::record_def`, direct
record or declaration metadata, member typedef arrays, namespace context id,
or an already-available `QualifiedNameKey`. The next packet must not introduce
or rely on a semantic helper that takes rendered `owner::member` text,
`std::string`, or `std::string_view` and parses it back into owner/member
identity.

## Watchouts

Non-semantic string uses remain in token-spelling/rendering helpers,
diagnostics/debug bridges, namespace compatibility projection, template debug
arg rendering, and final spelling projection. `TypeSpec::record_def`,
qualified record/tag probes, member typedef arrays, and existing structured
typedef maps remain the authority where present.

Do not remove the live `owner::member` mirror as a standalone deletion; the
failed exploratory proof showed it is still protecting parser progress and one
runtime member-typedef case.

Do not revive the rejected route under a different name. A helper such as
`find_structured_member_typedef_type(std::string_view qualified_name)` is not
structured progress if its first semantic step is parsing rendered qualified
lookup text or reconstructing owner/member identity from spelling.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` contains the fresh passing proof for this packet: 100% tests
passed, 0 tests failed out of 928.
