# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 direct record/type-head cleanup removed rendered record-tag authority from
`types_helpers.hpp` direct and qualified type-start probes:

- removed `has_defined_struct_tag(...)` fallback checks from
  `is_known_simple_type_head(...)`, `resolve_qualified_known_type_name(...)`,
  and `probe_qualified_type(...)`;
- replaced those direct/visible record checks with record-node metadata
  matching on namespace context plus `unqualified_text_id`;
- removed rendered `resolved_name` typedef re-entry from
  `visible_type_result_has_structured_record_definition(...)` and
  `qualified_type_structured_record_definition(...)`;
- added parser coverage showing rendered-only tag storage no longer authorizes a
  direct record type-head probe, while structured record metadata still does
  even when the rendered map key/name disagrees.

## Suggested Next

Next executor packet can continue Step 2 by auditing remaining parser semantic
lookup bridges outside this direct record/type-start helper path, especially
string projection overloads or `fallback`-named helpers that still feed parser
semantic lookup after a structured key exists.

## Watchouts

`resolve_record_type_spec(...)` still keeps the documented tag-only
sizeof/alignof/offsetof compatibility fallback in `support.cpp`; this packet
did not remove that non-type-start layout bridge because the existing typed
`TypeSpec::record_def` path already wins there and current tests still cover
the tag-only fallback contract.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
