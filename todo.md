# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 traced the NTTP default-expression reference route and recorded a
parser/Sema-owned metadata blocker instead of widening the packet. The exact
missing carrier is a TextId/structured NTTP binding environment for deferred
default-expression evaluation: `Parser::eval_deferred_nttp_default` and
`Parser::eval_deferred_nttp_expr_tokens` receive NTTP bindings only as
`std::vector<std::pair<std::string, long long>>`, while the AST default payload
is still `Node::template_param_default_exprs` rendered text plus parser-local
token caches. Because the evaluator's primary identifier path compares
`token_spelling(tok)` to rendered binding names, a drifted rendered name cannot
be proven subordinate to parser-owned TextId metadata until that carrier is
added through the parser template default-expression binding boundary.

## Suggested Next

Continue Step 4 with a different ordinary parser-to-Sema call/reference
variant, or delegate a focused parser template metadata packet to add a
TextId-keyed NTTP binding carrier for deferred NTTP default-expression
evaluation before removing the rendered binding comparisons.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- Step 4 should preserve structured metadata that already exists. Do not add
  fallback spelling, rendered qualified-text parsing, helper-only renames, or
  string/string_view semantic lookup routes.
- The new forwarded NTTP carrier covers parser/Sema AST call-env binding for
  simple forwarded NTTP names, including environments where only structured
  text metadata is present. It does not claim cleanup of expression-string
  `$expr:` template arguments or HIR `NttpBindings` string compatibility.
- The known-function slice covers using-imported call references whose parser
  carrier is target base `TextId` plus target qualifier `TextId` metadata. It
  does not fix parser production of resolved namespace context ids for
  namespaced free-function declarations that are rendered as `ns::fn`; that
  should be treated as a separate parser/Sema carrier packet if needed.
- The NTTP default-expression blocker is specifically at
  `Parser::eval_deferred_nttp_default` ->
  `Parser::eval_deferred_nttp_expr_tokens`: callers pass rendered NTTP binding
  names, and the AST default-expression surface does not expose a structured
  expression node or TextId-keyed binding list for Sema-style lookup authority.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
