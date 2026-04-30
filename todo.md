# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from consteval `TypeSpec`
type-binding resolution when structured or `TextId` binding metadata is
available. `resolve_type` now tries the structured binding key first, then the
`TextId` mirror, treats either metadata miss as authoritative, and only uses the
legacy rendered `type_bindings` map when no binding metadata is present.
Focused frontend coverage now verifies structured and `TextId` metadata win
over stale rendered names, metadata misses reject the stale rendered fallback,
and no-metadata rendered compatibility still works.

## Suggested Next

Continue Step 3 by reviewing the remaining Sema/consteval rendered semantic
lookup routes that already have structured or `TextId` metadata, then delegate
one bounded route where fallback can be limited to no-metadata compatibility
without touching HIR or backend carriers.

## Watchouts

- Some existing parsed method-body positives still lack structured field
  `TextId` metadata, so instance-field lookup must keep rendered compatibility
  fallback when the structured field metadata chain is absent.
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
- Consteval `TypeSpec` resolution still depends on the recorded rendered-name
  to structured/`TextId` mirror maps to find metadata for a `TypeSpec` tag;
  this packet made those mirrors authoritative but did not add intrinsic
  `TypeSpec` identifier metadata.

## Proof

Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 464/464 tests green. Proof log: `test_after.log`.
