# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 made Sema function and overload lookup treat parser/Sema function
metadata as authoritative. `lookup_function_by_name`,
`lookup_ref_overloads_by_name`, and `lookup_cpp_overloads_by_name` now return
only structured-key results when a reference carries a valid function key or
using-value-alias target key; rendered `Node::name` fallback is reserved for
keyless references. Focused authority coverage now mutates rendered callee
spelling to legacy function/overload entries while preserving structured
metadata and proves the lookup boundary rejects that drift.

## Suggested Next

Continue Step 4 by auditing the remaining Sema lookup helpers for the same
pattern: a reference carries producer-side `TextId`, namespace, owner, or
declaration metadata but a miss can still reopen rendered compatibility lookup.

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
- Function and overload lookup is now stricter for references with valid
  function metadata. If a supported call path starts failing here, fix the
  parser/Sema declaration or reference metadata producer instead of
  reintroducing rendered-name fallback in these helpers.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
