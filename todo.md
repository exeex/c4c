# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 repaired the dependent member-typedef base carrier for
`is_signed<T> : is_signed_helper<T>::type`. Parser template type parameters now
carry structured typedef-like `TypeSpec` identity in simple type positions,
the dependent base-instantiation path substitutes structured type args from
template metadata/concrete args, evaluates NTTP default placeholders from the
template declaration's cached/default expression carrier instead of using
`TemplateArgRef::debug_text` as the semantic sentinel, restores the pending
`::type` member suffix after base instantiation, and resolves the resulting
member typedef to a concrete base `record_def`.

The lookup-authority regression now covers `is_signed<int>`,
`is_signed<unsigned int>`, and `is_signed<void>` instances, mutates pending
primary-template base debug text before manual instantiation to prove producer
independence, mutates rendered base/debug text after the structured
`record_def` check, and validates that Sema inherited static-member lookup
consumes the structured base key.

## Suggested Next

Continue Step 4 by using the now-structured dependent member-typedef base
coverage to remove or further narrow any remaining rendered static-member owner
acceptance for template-specialized references whose parser/Sema metadata is
present.

## Watchouts

- The source idea remains active; Step 3.3 closure is not source-idea closure.
- This packet touched parser carrier files for the dependent member-typedef
  base route, but did not reopen alias NTTP base carrier behavior.
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
- The dependent member-typedef base path where `is_signed<T>` inherits through
  `is_signed_helper<T>::type` now carries a concrete base `record_def` for the
  direct `arithmetic<T>::value` reduced route; do not reintroduce rendered
  `$expr:` carrier parsing here.
- This packet repairs the direct `arithmetic<T>::value` default-expression
  carrier used by the reduced `is_signed_helper<T>::type` path. The successful
  defaulted-NTTP base lane is driven by template parameter/default metadata and
  `TemplateArgRef` structural fields, not by a `$expr:` debug-text prefix. If
  the cached default token lookup misses, the declaration's stored
  `template_param_default_exprs` re-lex path remains retained compatibility,
  not completed structured cleanup. A broader trait-default evaluator where
  the trait providing `value` itself inherits that static member through
  another typedef base should remain a separate packet if it becomes the next
  frontier.
- `resolve_deferred_member_base()` still finds the member template primary from
  `resolved_member.tpl_struct_origin` via `qualified_name_from_text(...)` and
  TextId-based lookup because `TypeSpec` does not yet carry a structured
  template-origin key for that member-typedef route. Treat this as retained
  parser metadata debt/compatibility, not completed removal of rendered or
  text-keyed lookup from the path.
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
