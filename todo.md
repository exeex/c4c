# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 repaired the bounded parser deferred owner-member lookup consumer
around `lookup_struct_member_typedef_recursive_for_type`: record-member and
selected-specialization member typedef scans now prefer
`deferred_member_type_text_id`/member `TextId` matching before rendered
`deferred_member_type_name`. Focused parser coverage feeds a stale rendered
deferred member spelling through a typedef-backed template argument and proves
the valid member `TextId` still resolves the member typedef.

## Suggested Next

Continue Step 3.1 with a bounded consumer-repair packet for
`typespec_mentions_template_param`: when `deferred_member_type_text_id` is
populated and misses the referenced template parameter, the helper must not
fall back to rendered `deferred_member_type_name` dependency classification.
After that repair is proven, re-check any remaining parser/Sema
`deferred_member_type_name` uses before classifying them as display,
mangling/reference-string construction, or downstream HIR handoff only.

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
- `typespec_mentions_template_param` still needs a populated-metadata-miss
  repair: if `deferred_member_type_text_id` is valid but does not mention the
  template parameter, rendered `deferred_member_type_name` must not decide
  dependency classification as a fallback.
- `lookup_struct_member_typedef_recursive_for_type` member-name matching is now
  proven against stale rendered `deferred_member_type_name` when a valid member
  `TextId` is present. Apart from the still-open
  `typespec_mentions_template_param` populated-metadata-miss behavior, rendered
  deferred-member uses found in this packet appear to be parser
  display/reference-string construction or handoff payloads; do not classify
  them as semantic lookup removal without proving the concrete parser/Sema
  consumer.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (464 tests passed).
