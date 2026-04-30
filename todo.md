# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 repaired the bounded structured consteval `TypeSpec` carrier route
for function-template type parameters: `TypeSpec` now carries the owning
template namespace, owner `TextId`, parameter index, and parameter `TextId`
for parsed `sizeof(T)`-style template parameter references, and consteval type
binding resolution can build `TypeBindingStructuredKey` directly from that
intrinsic carrier before consulting rendered-name-to-key mirrors. Focused tests
cover parser preservation of the owner/index carrier and consteval lookup with
a stale rendered `TypeSpec::tag`, no `type_binding_keys_by_name`, and no
`type_bindings_by_text` map.

## Suggested Next

Continue Step 3.1 with one bounded metadata-producer packet for any remaining
synthetic local or qualified `TypeSpec` carrier gap not covered by the
function-template type-parameter owner/index route before attempting broad
consteval rendered-fallback deletion.

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

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (464 tests passed).
