# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 tightened Sema static-member lookup for generated/template-specialized
references. Template-specialized references with structured owner/member
metadata now treat existing structured static-member/base metadata as
authoritative: after such metadata is present, Sema no longer retries through
the rendered owner/member spelling on a structured miss. The lookup-authority
test now covers a templated static-member reference whose rendered owner names
a stale record while the structured owner key names a different templated
record with static-member metadata.

The remaining compatibility route is narrowed to template-specialized
references whose parser handoff still lacks structured static-member/base
metadata. The exact missing carrier observed during this packet is
`is_signed<T> : is_signed_helper<T>::type`: instantiated `is_signed_T_*` bases
still reach Sema as unresolved `is_signed_helper` template carriers with
debug `$expr:arithmetic<T>::value` argument text and no `record_def` base key
for inherited `value`. That carrier needs parser-side structured NTTP default
expression/base-record metadata before the final rendered owner acceptance can
be deleted safely.

## Suggested Next

Continue Step 4 by repairing the dependent member-typedef base carrier for
`is_signed<T> : is_signed_helper<T>::type`, specifically the structured NTTP
default expression `arithmetic<T>::value` and resulting `record_def` base key
for instantiated `is_signed_T_*` records.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- This packet did not touch parser carrier files or the alias NTTP base carrier.
  It only tightened Sema static-member lookup around template-specialized
  references when structured static-member/base metadata is already present.
- Ordinary global-scope C++ overload declarations are still a separate blocker
  because the current AST handoff does not let Sema distinguish them from C
  global redeclarations without weakening C behavior. Preserve C and
  `extern "C"` rejection until a structured language/linkage carrier exists.
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
  references are strict once Sema has structured static-member/base metadata;
  the remaining rendered acceptance is limited to template-specialized
  references with no such metadata.
- The remaining static-member blocker is not the alias-template deferred NTTP
  base carrier covered by `signed_probe`/`is_enum`. It is the dependent
  member-typedef base path where `is_signed<T>` inherits through
  `is_signed_helper<T>::type`, and the instantiated base still carries
  debug `$expr:arithmetic<T>::value` text instead of a structured NTTP/default
  and concrete base `record_def`.
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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build succeeded/up to date. CTest passed `886/886` matched tests,
including `frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the corrected
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
