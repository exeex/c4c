# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed the `infer_expr` var-ref compatibility retry that re-entered
`lookup_symbol(n->unqualified_name, n)` after a miss on rendered `Node::name`.
Local var-ref Sema lookup now relies on the parser-produced structured local
name key already carried by `unqualified_text_id`. A focused parsed authority
test now mutates only the rendered local reference spelling and proves Sema
validates through the declaration/reference TextId handoff.

## Suggested Next

Continue Step 4 with the next parser/Sema handoff gap that still has
producer-side `TextId`, namespace, owner, or declaration metadata available but
also keeps rendered compatibility lookup in the semantic path.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- Step 4 should preserve structured metadata that already exists. Do not add
  fallback spelling, rendered qualified-text parsing, helper-only renames, or
  string/string_view semantic lookup routes.
- The var-ref path now depends on parser local declarations and references
  carrying `unqualified_text_id`; if a future local-like producer has no TextId
  carrier, fix that producer instead of reintroducing a rendered retry in Sema.
- The deferred NTTP default-expression carrier covers parser-local evaluation
  of simple NTTP identifier references where producer template parameter
  metadata exists. It does not claim cleanup of expression-string `$expr:`
  template arguments or HIR `NttpBindings` string compatibility.
- The known-function slice covers using-imported call references whose parser
  carrier is target base `TextId` plus target qualifier `TextId` metadata. The
  namespaced free-function slice covers namespace-resolved qualified
  declarators, but does not claim cleanup of class-member declarator ownership
  or HIR/module declaration lookup compatibility.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
