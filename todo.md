# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.3
Current Step Title: Remove Remaining Sema Owner/Member/Static Rendered Routes
你該做test baseline review了

## Just Finished

Step 3.3 fixed the `e0b01f49` Sema symbol-lookup regression by keeping rendered
global/enum fallback rejection limited to references with no qualifier segments:
unqualified names and global-qualified (`::name`) references still reject stale
rendered entries when structured metadata exists, while namespace/owner-qualified
rendered bridges such as the global-scope using-declaration path retain
compatibility. The existing focused stale global-qualified rendered global and
enum tests remain covered.

## Suggested Next

Choose the next bounded Step 3 rendered-lookup removal packet from the remaining
watchouts, keeping it to one concrete producer gap or one proven consumer
deletion route; namespace-qualified rendered globals should stay parked until
their producer metadata is equivalent.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
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
- Function/ref-overload/C++ overload rendered-name compatibility now treats
  structured metadata on qualified rendered legacy entries as authoritative
  after a valid structured reference key misses. Do not reintroduce an
  unqualified-only rendered-name guard for these helpers.
- `supports_cpp_overload_set` now accepts global-qualified `::operator_*`
  spellings so the qualified rendered C++ overload path is exercised; broader
  owner-qualified operator registration remains outside this packet because
  owner-qualified function names are still routed as method-owner forms.
- Global value and enum-constant lookup now reject stale `::...` rendered
  compatibility after a valid structured key miss when the rendered entry has
  structured metadata. Owner/namespace-qualified rendered compatibility such as
  using-import and anonymous-namespace bridges is intentionally retained until
  those producers carry equivalent structured qualifier metadata.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_positive_sema_using_global_scope_decl_parse_cpp)$' --output-on-failure) > test_after.log 2>&1`

Proof log: regenerated canonical `test_after.log` (465 tests passed).
