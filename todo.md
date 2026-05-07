Status: Active
Source Idea Path: ideas/open/146_qualified_name_deferred_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Unqualified `TextId` APIs From Structured Qualified APIs

# Current Packet

## Just Finished

Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs
started the parser typedef/type migration. `Parser::has_typedef_name(TextId)`,
`Parser::has_typedef_type(TextId)`, `Parser::find_typedef_type(TextId)`,
`Parser::resolve_visible_type(TextId)`, `Parser::lookup_type_in_context(...)`,
and `Parser::struct_typedef_key_in_context(...)` now reject spellings
containing `::` instead of splitting rendered compound names into semantic
lookup authority.

Compound typedef/type lookup remains available through existing structured
paths: `Parser::find_typedef_type(QualifiedNameKey)` and
`Parser::resolve_qualified_type(QualifiedNameRef)`. The current-record sibling
fallback was also converted from a rendered `StructuredNs::Sibling` `TextId`
probe to a context-derived `QualifiedNameKey` probe.

Focused tests now assert that rendered `TextId` typedef probes fail for
compound spellings, structured qualified keys still recover qualified typedefs,
and stale rendered typedef storage does not re-enter parser type authority.

## Suggested Next

Continue Step 2 with the next parser family that still allows rendered
compound `TextId` lookup authority, likely value/function lookup and
registration wrappers around `known_fn_name_key_in_context()`,
`lookup_value_in_context()`, and `register_var_type_binding()`, while keeping
structured `QualifiedNameRef` / `QualifiedNameKey` value lookup intact.

## Watchouts

Do not promote rendered compound typedef writes into structured keys as a
general rule; the first proof run showed that doing so turns stale rendered
fixtures into real semantic authority. Structured tests should seed structured
keys explicitly. Value/function compatibility helpers still use
`qualified_key_in_context()` on rendered `TextId` spellings and remain for a
separate packet.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_positive_sema_namespace_using_decl_typedef_and_value_frontend_cpp|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp)$'; } > test_after.log 2>&1`

Proof passed. `test_after.log` contains the canonical executor proof log.
