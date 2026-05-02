# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 added a parser/Sema-owned `ParserNttpBindingMetadata` carrier for
deferred NTTP default-expression evaluation. `Parser::eval_deferred_nttp_default`
and `Parser::eval_deferred_nttp_expr_tokens` now accept optional structured NTTP
binding metadata and consult TextId/key metadata before rendered binding names;
when authoritative metadata exists and misses, the evaluator no longer reopens
the rendered-name NTTP binding route. Parser template instantiation handoff
sites now build and forward this metadata from template parameter TextIds while
preserving the existing compatibility binding list.

## Suggested Next

Continue Step 4 with the next parser/Sema handoff gap, preferably a remaining
ordinary call/reference or template argument route that still has a compatibility
string mirror but already has producer-side TextId/key metadata available.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- Step 4 should preserve structured metadata that already exists. Do not add
  fallback spelling, rendered qualified-text parsing, helper-only renames, or
  string/string_view semantic lookup routes.
- The deferred NTTP default-expression carrier covers parser-local evaluation
  of simple NTTP identifier references where producer template parameter
  metadata exists. It does not claim cleanup of expression-string `$expr:`
  template arguments or HIR `NttpBindings` string compatibility.
- The known-function slice covers using-imported call references whose parser
  carrier is target base `TextId` plus target qualifier `TextId` metadata. It
  does not fix parser production of resolved namespace context ids for
  namespaced free-function declarations that are rendered as `ns::fn`; that
  should be treated as a separate parser/Sema carrier packet if needed.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
