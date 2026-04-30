# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 qualified-type parse fallback repair completed the reviewer-blocked
route cleanup from `review/qualified_type_parse_fallback_review.md`:

- removed rendered qualified enum lookup from `probe_qualified_type`; qualified
  enum authority now comes from parsed enum definition metadata in namespace
  context, not `find_typedef_type(find_parser_text_id("Q::E"))`;
- removed the spelling-only unresolved `Q::T<...>` type path and kept only the
  existing dependent-template placeholder case when parsed template arguments
  reference an active template type parameter;
- kept structured typedef registration on namespace-scope typedef producers and
  removed the block-local registration leak;
- added focused parser tests for legacy rendered enum storage, unknown
  namespace-qualified template ids, parsed namespace-scope typedef
  registration, and block-local typedef non-visibility through `ns::T`.

## Suggested Next

Next executor packet can continue Step 2 by auditing the remaining parser
rendered-string semantic lookup routes outside the repaired qualified-type parse
path, using the same rule: structured metadata may authorize parsing; rendered
spelling alone may not.

## Watchouts

The dependent-template placeholder intentionally accepts `Q::T<U>` only when a
parsed template argument is an active template type parameter. Unknown
namespace-qualified template ids with concrete arguments, such as
`ns::Missing<int>`, remain rejected.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
