# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 qualified-type parse fallback slice was reviewed in
`review/qualified_type_parse_fallback_review.md` and rejected as route drift,
not accepted progress. The explicit unresolved qualified fallback bit was
removed, but the dirty implementation still preserves equivalent rendered
semantic authority through replacement paths:

- qualified enum probing still renders `Q::E`, converts it back through parser
  text lookup, and accepts the resulting enum `TypeSpec`;
- unresolved qualified template-id parsing still accepts rendered `Q::T<...>`
  spelling without structured template/dependent authority;
- local typedef producer changes can leak block-local typedefs into namespace
  structured lookup authority.

This packet is incomplete until those replacement paths are removed, converted
to real structured metadata, or split into a metadata blocker idea. The source
idea's no-rendered-semantic-authority rule remains controlling: if structured
metadata is absent, do not infer parser type authority from rendered spelling.

## Suggested Next

Next executor packet stays on Step 2 and repairs qualified-type parsing before
any broader parser route is attempted:

1. Remove rendered qualified enum authority from `probe_qualified_type`; enum
   acceptance must come from structured qualified metadata, an already attached
   record/tag carrier, or a clearly scoped metadata-producer repair.
2. Remove or narrow the unresolved qualified template-id side path so
   `Q::T<...>` is not accepted from spelling alone. Accept only cases backed by
   structured template/dependent authority, or split the missing carrier into a
   new open metadata idea.
3. Repair typedef structured registration so namespace-scope typedefs are
   registered without making block-local typedefs visible through namespace
   qualified lookup.
4. Add same-feature tests for legacy rendered enum storage, unknown
   namespace-qualified template ids, parsed namespace-scope typedef
   registration, and block-local typedef non-visibility through `ns::T`.

Do not commit the current dirty implementation slice as completed Step 2
progress unless these points are addressed and the proof is rerun.

## Watchouts

Reject over-broad replacement paths even if they make the focused testcase
green. A helper rename, a moved parse fallback, or a new structured-looking
registration that authorizes the wrong scope is not lookup removal.

`TextId` by itself is only text identity. Qualified type parsing that needs
namespace, tag, typedef, template, dependent, or declaration meaning must use a
domain carrier such as `QualifiedNameKey`, namespace context id, declaration or
record/tag identity, or explicit dependent/template metadata.

## Proof

Rejected dirty slice proof, not acceptance proof:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result reported by the rejected executor slice: passed, 927/927 tests green.
Reviewer judgment: green subset is insufficient because the implementation
preserves rendered semantic authority through replacement paths. The next
executor must rerun the delegated proof after the route repairs.
