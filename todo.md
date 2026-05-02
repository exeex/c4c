# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 audited the remaining `n_template_args > 0` static-member compatibility
route. Removing the Sema retry through `structured_record_key_for_tag(tag)` and
switching concrete template-member references to the instantiated owner exposed
a missing parser-to-Sema carrier for inherited alias-template static members:
`signed_probe<int>::value` and `is_enum<Color>::value` arrive with concrete
reference owners, but their inherited base TypeSpecs still carry rendered
deferred tags such as `integral_constant_bool_$expr:(T(-1)<T(0))` with no
`record_def` or structured template-argument carrier Sema can traverse.
The experimental code changes were backed out; no rendered owner/member lookup
authority was added.

## Suggested Next

Continue Step 4 by adding a structured parser carrier for alias-template
deferred NTTP base instantiations before removing the template-specialization
static-member compatibility route. The next coherent packet should make
instantiated records such as `signed_probe_T_int` expose a base `record_def` or
equivalent typed owner/argument metadata for `integral_constant<bool, expr>`.

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
- Removing the `n_template_args > 0` static-member fallback currently regresses
  `cpp_positive_sema_template_alias_deferred_nttp_expr_runtime_cpp` and
  `cpp_positive_sema_template_builtin_is_enum_inherited_value_runtime_cpp`.
  The missing carrier observed during audit is the inherited base TypeSpec for
  alias-template deferred NTTP expressions: it keeps `$expr:` rendered tag text
  but lacks `record_def` or structured template-argument metadata after
  substitution.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.
- `review/step33_final_route_review.md` and prior review artifacts are
  transient review files and were not modified by this lifecycle update.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_)' | tee test_after.log`.

Result: build succeeded and the filtered CTest subset passed
`885/885` tests on the restored worktree. During the audit, the attempted
removal/replacement failed the same delegated subset at
`cpp_positive_sema_template_alias_deferred_nttp_expr_runtime_cpp` and
`cpp_positive_sema_template_builtin_is_enum_inherited_value_runtime_cpp`, which
identified the missing alias-template deferred NTTP base carrier recorded
above. Final proof log path: `test_after.log`.
