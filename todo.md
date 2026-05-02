# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the alias-template deferred NTTP base carrier so
`TemplateArgRef` value arguments carry parser-owned `nttp_text_id` identity.
Alias substitution now maps structured value carriers through that field, not
through `TemplateArgRef::debug_text`, while preserving the parsed expression
node and instantiated inherited-base `record_def` attachment. The focused
lookup-authority test now seeds a stale value-carrier debug spelling before
alias substitution and proves the structured `TextId` still selects the actual
NTTP argument and retains the base `record_def`.

## Suggested Next

Continue Step 4 with a supervisor-chosen review or next narrow cleanup of the
remaining template-specialization static-member compatibility route. Do not
expand this completed carrier repair into expectation rewrites or new rendered
lookup paths.

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
  handled in the next focused packet if Step 4 continues there.
- The new base carrier intentionally preserves structured value-argument
  payloads only where pending template args contain NTTP values; type-only
  pending template structs keep the prior debug carrier to avoid widening into
  unrelated nested-template owner work.
- `TemplateArgRef::debug_text` remains display/recovery text only for the
  structured alias NTTP value route. Retained `$expr:` split/eval blocks are
  compatibility fallback for expression carriers that do not yet have a fully
  structural parse/eval path; they are not completed deletion of the legacy
  compatibility route.
- Debug `$expr:` text remains available as fallback display/recovery text, but
  the successful `signed_probe`/`is_enum` base `record_def` route is seeded from
  parsed `TemplateArgRef` value carriers and expression nodes, not by parsing or
  splitting rendered `$expr:` tags.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests with the structural NTTP carrier repair. Final proof log path:
`test_after.log`.
