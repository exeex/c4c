# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 qualified-type parse fallback repair completed the reviewer-blocked
route cleanup from `review/qualified_type_parse_fallback_review.md` by removing
the remaining `Parser::lookup_type_in_context` projected rendered typedef
fallback:

- removed the `bridge_name_in_context(...)` / rendered `find_typedef_type(...)`
  semantic authority path after structured namespace typedef lookup misses;
- kept direct namespace, imported namespace, and global visible type lookup
  working when the typedef authority comes from structured keys;
- updated focused parser tests so legacy rendered `ns::T` typedef storage alone
  cannot drive `lookup_type_in_context` or qualified type resolution, while
  parsed namespace-scope typedefs still resolve and block-local typedefs do not
  leak through `ns::T`.

## Suggested Next

Next executor packet can continue Step 2 by auditing the remaining parser
rendered-string semantic lookup routes outside the repaired qualified-type and
namespace typedef lookup paths, using the same rule: structured metadata may
authorize parsing; rendered spelling alone may not.

## Watchouts

The dependent-template placeholder intentionally accepts `Q::T<U>` only when a
parsed template argument is an active template type parameter. Unknown
namespace-qualified template ids with concrete arguments, such as
`ns::Missing<int>`, remain rejected. `lookup_type_in_context(0, T)` can still
report the queried visible spelling for an imported structured typedef through
the existing global visible typedef facade; non-global imported namespace lookup
projects the structured `ns::T` spelling.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
