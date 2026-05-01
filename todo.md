# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 classified the local variable declaration producer family for
`parser_text_id_for_token(TextId token_text_id, std::string_view fallback)`.
The `parse_local_decl` and `if`/`while`/`switch` condition-declaration sites
were semantic value-binding producers: they copied declarator names into
`Node::unqualified_text_id` and immediately registered local value/type
bindings from that field. `parse_declarator` already returns the token-backed
`out_name_text_id` for unqualified declarator names, so these declarations now
store only `vname_text_id`; metadata-less or non-unqualified declarators remain
unbound instead of reinterning the rendered `vname`. The range-for rebinding
path also now reuses `decl->unqualified_text_id` instead of recovering a
`TextId` with `find_parser_text_id(decl->name)`.

## Suggested Next

Continue Step 2.3 by classifying the next `parser_text_id_for_token` producer
family. Good candidates are the remaining `kInvalidText` call sites in
`declarations.cpp`, `types/struct.cpp`, `types/template.cpp`, and
`types/types_helpers.hpp`; remove or narrow only a semantic fallback use where
token/structured `TextId` metadata is already available, otherwise record the
family as display-only, synthetic/prelude, or blocked by missing metadata.

## Watchouts

- The function-parameter scope guard fallback route is removed for the covered
  parser-owned parameter declarations. If a future parameter-like producer
  lacks `Node::unqualified_text_id`, treat that as missing producer metadata
  instead of restoring `parser_text_id_for_token(kInvalidText, param->name)` in
  the binding guard.
- Local declaration and condition-declaration value binding now depends on the
  declarator-produced `vname_text_id`. If a future parser-owned local needs a
  binding but arrives with `vname_text_id == kInvalidText`, fix the declarator
  metadata producer instead of restoring `parser_text_id_for_token(kInvalidText,
  vname)` or `find_parser_text_id(decl->name)`.
- `cache_typedef_type(const std::string& name, ...)` still exists, but its
  remaining direct callers are the synthetic/prelude typedef aliases registered
  in `core.cpp` (`va_list`, fixed-width integer aliases, `std::__true_type`,
  `std::__false_type`, etc.). The declarator dependent-member typedef family is
  no longer a string-keyed cache caller.
- Do not restore parser tests that prove dependent member typedef lookup by
  recovering a `TextId` from rendered names such as `Alias::type` or
  `Root::Nested::type`; the covered route should be proven through
  `QualifiedNameKey`.
- `parser_text_id_for_token`, `find_parser_text_id`, `parser_text`,
  `token_spelling`, `set_parser_owned_spelling`, debug/diagnostic helpers,
  test-only hooks, and AST final spelling builders are not automatically Step 2
  violations. Classify them by concrete semantic lookup call path before
  deleting or renaming them.
- `visible_name_spelling(const VisibleNameResult&)` is intentionally the final
  spelling/display projection boundary, not a semantic lookup route.
- `compatibility_namespace_name_in_context` no longer exists as parser API.
  Its final-spelling compatibility behavior is confined to
  `render_lookup_name_in_context`; do not reintroduce it as a public/class
  fallback helper.
- `bridge_name_in_context(int, TextId, std::string_view fallback_name)` no
  longer exists as parser API. The remaining public replacement is
  `render_name_in_context(int, TextId)`, which is a final-spelling renderer;
  do not reintroduce fallback spelling on that public route.
- `canonical_name_in_context(int, const std::string& name)` no longer exists as
  parser API. Do not reintroduce string-taking namespace/name construction when
  `TextId`-based final spelling via `render_name_in_context` is sufficient.
- `ensure_named_namespace_context(int, TextId, const std::string&)` no longer
  exists as parser API. Named namespace construction is keyed only by `TextId`;
  retained display/canonical namespace spelling is derived from
  `parser_text(text_id, {})` after the semantic key is known.
- `register_tag_type_binding` no longer accepts a rendered string as its
  semantic binding key. The `const char* tag` argument is still retained as
  `TypeSpec` compatibility/display payload; do not treat that payload as lookup
  authority while the `TextId` binding key and `tag_text_id` are available.
- Do not accept a parser public semantic API that keeps `std::string`,
  `std::string_view`, `const char*`, `fallback_name`, `compatibility`, or
  `legacy` as a lookup key/escape hatch when `TextId` or a domain key exists.
- Do not delete or narrow derived/static-member rendered compatibility yet:
  `cpp_positive_sema_inherited_static_member_lookup_simple_runtime_cpp` still
  needs the current no-metadata fallback when structured base-chain/static-member
  metadata is absent.
- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` before
  treating it as parser/Sema closure work.
- The `ConstEvalEnv::lookup(const Node*)` NTTP string bridge is intentionally
  retained only for HIR-owned `nttp_bindings` compatibility when the NTTP
  structured/TextId metadata maps themselves have not missed.
- No-metadata consteval value rendered compatibility remains intentional for
  producers with no structured/TextId value metadata maps.
- Type-binding rendered bridges and HIR-owned `NttpBindings` compatibility were
  not removed in this packet.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.
- Namespace-qualified rendered bridges and synthetic locals without structured
  metadata remain compatibility candidates; do not collapse those routes until
  their producers carry equivalent structured metadata.
- Consteval `TypeSpec` resolution no longer needs rendered-name-to-`TextId`
  mirror maps or rendered-name-to-key mirrors for the covered unqualified
  function-template type-parameter `sizeof(T)` route, but compatibility
  remains for routes whose `TypeSpec` producers do not yet carry equivalent
  owner/index metadata.
- Consteval interpreter locals may still need same-spelling rendered
  compatibility for synthetic local producers not covered by parameter,
  condition-local, ordinary-local, or `for`-init local declaration metadata.
  Treat removal of that compatibility as a metadata-producer packet, not a
  lookup-only packet.
- Qualified `TypeSpec` now has a parser-owned carrier for qualifier `TextId`s
  and namespace context on the covered qualified base-type parser path, but
  retained rendered qualifier strings are still compatibility/display payloads
  until each downstream consumer is proven against the structured fields.
- Qualified `typename` structured typedef payload copies now receive the same
  parser-owned qualifier/base metadata, but broader deferred template
  owner-member handoff remains unproven and should stay as a separate bounded
  producer packet if needed.
- Deferred owner-member `TypeSpec` carriers now preserve the member `TextId`
  for the covered parser-owned production routes, but the legacy rendered
  member spelling is still present for compatibility/display. Do not delete
  rendered deferred-member compatibility until each downstream consumer is
  proven against `deferred_member_type_text_id` or an equivalent structured
  owner/member carrier.
- `lookup_struct_member_typedef_recursive_for_type` member-name matching is now
  proven against stale rendered `deferred_member_type_name` when a valid member
  `TextId` is present.
- Template-instantiation type-argument deferred-member resolution now accepts
  the member `TextId` as the lookup source even when the rendered
  deferred-member spelling is absent; keep remaining rendered `$member`
  template-arg strings as compatibility/debug payloads until a concrete
  semantic consumer is proven.
- `typespec_mentions_template_param` dependency classification is now proven
  against stale rendered `deferred_member_type_name` when a valid
  `deferred_member_type_text_id` is present. Remaining rendered deferred-member
  uses found in this route appear to be parser display/reference-string
  construction or handoff payloads; do not classify them as semantic lookup
  removal without proving the concrete parser/Sema consumer.
- `type_binding_values_equivalent` no longer accepts rendered deferred-member
  spelling equality when either side has populated member TextId metadata and
  the other side lacks or mismatches it. Same-TextId equality still tolerates
  stale rendered member spelling.
- Derived/static-template static-member routes can still lack structured
  static-member table or base-chain metadata even when the reference has
  owner/member `TextId`s; keep rendered compatibility there until that producer
  gap is closed. The retained path is explicitly no-metadata compatibility, not
  a rendered-string authority after structured static-member metadata exists.
- Function/ref-overload/C++ overload helpers no longer eagerly consult rendered
  maps for comparison when a valid structured reference key exists. Do not
  reintroduce an eager rendered probe or an unqualified-only rendered-name guard
  for these helpers.
- `lookup_consteval_function_by_name` no longer reads `consteval_funcs_` before
  structured/TextId metadata when a valid carrier exists. Keep the rendered
  consteval function map as no-metadata compatibility only; do not reintroduce
  rendered comparison probes on metadata-backed references.
- `supports_cpp_overload_set` now accepts global-qualified `::operator_*`
  spellings so the qualified rendered C++ overload path is exercised; broader
  owner-qualified operator registration remains outside this packet because
  owner-qualified function names are still routed as method-owner forms.
- Global value and enum-constant lookup now reject stale `::...` rendered
  compatibility after a valid structured key miss when the rendered entry has
  structured metadata. Owner/namespace-qualified rendered compatibility such as
  using-import and anonymous-namespace bridges is intentionally retained until
  those producers carry equivalent structured qualifier metadata.
- Method-owner lookup now trusts qualifier TextId metadata over stale rendered
  owner spelling when the resulting Sema owner key resolves to an existing
  record. Namespace-qualified out-of-class methods can still arrive with an
  incomplete owner key, so their rendered compatibility remains intentionally
  retained until that producer metadata is complete.
- Method-body instance-field lookup now trusts structured owner/member metadata
  before rendered field spelling. The rendered fallback is intentionally still
  present for no-metadata compatibility only.
- Static-member lookup no longer eagerly consults rendered owner/member spelling
  once valid structured owner/member metadata is available. The rendered
  fallback is intentionally still present for no-metadata compatibility only.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_positive_sema_using_global_scope_decl_parse_cpp)$' --output-on-failure) > test_after.log 2>&1`

Proof log: regenerated canonical `test_after.log` after this packet (465 tests
passed).
