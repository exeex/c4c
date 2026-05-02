# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the parsed static-member lookup handoff for concrete
owner/member references. Parser qualified-name metadata now records the owner
namespace context for class-owner qualifiers, template-instantiated records and
their cloned fields carry structured record/member `TextId` metadata, and Sema
now treats a non-template static-member reference with a valid qualifier owner
key plus member `TextId` as authoritative instead of retrying through
`structured_record_key_for_tag(tag)` or rendered owner spelling. Focused
authority coverage mutates rendered static owner/member spelling while
preserving qualifier/member metadata, and strengthens the stale rendered-owner
negative case so the old same-member fallback would have accepted it.

## Suggested Next

Continue Step 4 by auditing the remaining template-specialization static-member
compatibility routes. This slice kept existing `n_template_args > 0`
specialization behavior intact while removing the concrete non-template
rendered-owner fallback.

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
- Method-owner lookup now depends on parser qualifier metadata or direct record
  child ownership. Qualified function templates with parser qualifier metadata
  remain outside the C-style redeclaration signature table to avoid reviving
  the prior rendered-name skip as namespace-function conflict behavior.
- Static-member lookup is now strict for concrete references with parser owner
  qualifier metadata plus member `TextId`. Generated template-specialization
  references still use their established compatibility route and should be
  handled in a separate focused packet if Step 4 continues there.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests. Fresh proof log: `test_after.log`.
