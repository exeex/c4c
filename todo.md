# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 expression qualified type-owner disambiguation cleanup removed rendered
owner-name semantic authority from
`qualified_name_starts_from_type_owner(...)`:

- replaced the rendered `owner_name` route through
  `has_typedef_type(find_parser_text_id(owner_name))` and
  `defined_struct_tags.count(owner_name)` with structured owner authority from
  the owner `QualifiedNameRef`;
- added `qualified_type_owner_has_structured_authority(...)` so owner checks
  accept direct visible unqualified typedefs, structured namespace typedef keys,
  template struct metadata, specializations, and namespace-context record
  definitions without re-querying rendered owner spelling;
- added parser coverage showing legacy rendered `ns::LegacyOwner` typedef/tag
  storage alone no longer authorizes a qualified functional-cast parse, while
  structured owner record metadata still authorizes the owner route.

## Suggested Next

Next executor packet can continue Step 2 by auditing the remaining parser
rendered-string semantic lookup routes in qualified type/member parsing helpers,
especially compatibility fallbacks that still consult `struct_tag_def_map` or
rendered typedef names after structured owner resolution misses.

## Watchouts

The new focused expression test keeps the positive structured-owner assertion
at the owner-authority layer because the adjacent member-template functional
cast parse path still has separate member-type resolution constraints. That
appears to be a follow-on parser helper cleanup rather than a reason to keep
rendered owner authority in expression disambiguation.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
