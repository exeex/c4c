# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.2
Current Step Title: Add Template-Instantiation Member-Typedef Carrier

## Just Finished

Step 2.4.4.2 implementation completed. Parser template state now has structured
storage keyed by `TemplateInstantiationKey concrete_owner + TextId
member_text_id`, exposed through parser register/find APIs.

The template-instantiation member-typedef writer in
`src/frontend/parser/impl/types/base.cpp` now registers substituted member
typedefs through that concrete-instantiation carrier instead of calling
`register_struct_member_typedef_binding(mangled, member, type)`. The reader
path for instantiated record owners reconstructs the same concrete owner key
from the instantiated record's primary-template metadata and concrete argument
payload, then queries by member `TextId` before consulting record member arrays.

Focused parser coverage now corrupts stale rendered scoped typedef storage and
the direct member array after instantiation, then proves
`Box<PayloadAlias>::Alias` still resolves through the structured concrete-owner
carrier.

## Suggested Next

Next bounded packet: Step 2.4.4.3 can re-audit whether any semantic
`owner::member` typedef mirror consumers remain, then shrink or delete the
rendered scoped mirror only if all reachable consumers have structured carriers
or explicit blockers.

## Watchouts

The new carrier is parser-local. Do not use it as proof that HIR/LIR/BIR or
backend metadata boundaries can consume concrete-instantiation member typedefs
without their own structured handoff.

`register_struct_member_typedef_binding(owner, member, type)` still exists for
remaining mirror audit work; Step 2.4.4.3 should verify live semantic readers
before deleting or narrowing it.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_parse_local_using_alias_statement_probe_dump|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Passed: build plus 928 selected frontend/parser/Sema tests. Proof log:
`test_after.log`.
