# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 repaired a bounded consteval `TypeSpec` metadata producer route:
`TypeSpec` now carries parser-owned `tag_text_id` metadata for typedef/template
parameter names, synthesized template-parameter typedef bindings populate that
carrier, and consteval type binding resolution can use `TypeSpec::tag_text_id`
directly against `type_bindings_by_text` without rendered-name-to-`TextId`
mirror maps for `sizeof(T)`-style template parameter TypeSpecs. Focused tests
cover parser preservation of the carrier and consteval lookup with a stale
rendered `TypeSpec::tag` while no rendered-name text mirror is installed.

## Suggested Next

Continue Step 3.1 with one bounded metadata-producer packet for any remaining
synthetic local or qualified/structured `TypeSpec` carrier gap before
attempting broad consteval rendered-fallback deletion.

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
  mirror maps for the covered unqualified template-parameter `sizeof(T)` route,
  but structured-key lookup still uses the existing rendered-name-to-key mirror
  because `TypeSpec` does not yet carry the function-template owner/parameter
  index needed to build `TypeBindingStructuredKey` intrinsically.
- Consteval interpreter locals may still need same-spelling rendered
  compatibility for synthetic local producers not covered by parameter,
  condition-local, ordinary-local, or `for`-init local declaration metadata.
  Treat removal of that compatibility as a metadata-producer packet, not a
  lookup-only packet.

## Proof

Passed:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log` (464 tests passed).
