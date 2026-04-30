# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 repaired the parser template-instantiation base-type deferred
owner-member consumer in `parse_base_type`: instantiated base `TypeSpec`
resolution now derives the lookup member spelling from populated
`deferred_member_type_text_id` before falling back to rendered
`deferred_member_type_name`. Focused parser coverage proves a deferred base
member typedef carrier with member TextId metadata but no rendered member
spelling still resolves through the structured member identity.

## Suggested Next

Continue Step 3.1 by routing the next concrete parser/Sema deferred-member or
consteval metadata consumer through populated structured/TextId metadata before
attempting any rendered-compatibility deletion.

## Watchouts

- Do not treat the previous consteval value/type fallback slice as closed while
  same-spelling rendered fallback can still decide lookup after populated
  metadata misses.
- Same-spelling consteval local/loop compatibility remains a metadata-producer
  target, not acceptable lookup-deletion progress.
- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
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
- Template-instantiation base-type deferred-member resolution now accepts the
  member `TextId` as the lookup source even when the rendered deferred-member
  spelling is absent; keep remaining rendered `$member` template-arg strings as
  compatibility/debug payloads until a concrete semantic consumer is proven.
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

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (464 tests passed).
