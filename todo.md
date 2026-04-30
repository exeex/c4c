# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4
Current Step Title: Audit Parser Type, Tag, And Member-Typedef Routes

## Just Finished

Step 2.4 is complete for this packet. The parser type/tag/member-typedef audit
covered the delegated parser-owned files and found one removable compatibility
wrapper: the unused `register_struct_member_typedef_binding(const std::string&,
const TypeSpec&)` overload that split a rendered scoped name into owner/member
semantic identity. That overload was deleted from `parser.hpp` and `core.cpp`.

The broader `register_struct_member_typedef_binding(owner, member, type)`
rendered `owner::member` typedef mirror was tested as a possible removal, but
the delegated proof exposed reachable dependence: removing it caused parser
timeouts in `eastl_slice7_piecewise_ctor_parse`,
`step3_timeout_probe_baseline_parse`, and `tuple_element_alias_mix_parse`, plus
a runtime failure in `template_variable_alias_member_typedef_runtime`. That
route remains a real Step 2.4 blocker until those consumers are structurally
routed through owner/member metadata.

## Suggested Next

Delegate a narrow follow-up for the remaining member-typedef blocker: replace
the live `owner::member` typedef mirror consumers with structured owner/member
lookup in parser type parsing before deleting the mirror.

## Watchouts

Non-semantic string uses remain in token-spelling/rendering helpers,
diagnostics/debug bridges, namespace compatibility projection, template debug
arg rendering, and final spelling projection. `TypeSpec::record_def`,
qualified record/tag probes, member typedef arrays, and existing structured
typedef maps remain the authority where present.

Do not remove the live `owner::member` mirror as a standalone deletion; the
failed exploratory proof showed it is still protecting parser progress and one
runtime member-typedef case.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

`test_after.log` contains the fresh passing proof for this packet: 100% tests
passed, 0 tests failed out of 928.
