# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from Sema consteval function lookup
after structured or `TextId` metadata makes a miss authoritative. The
consteval interpreter and static-assert validator still prefer structured and
`TextId` hits, but stale rendered-name consteval functions no longer survive
when those metadata routes miss. Focused tests cover stale rendered disagreement
being rejected after metadata misses and the no-metadata rendered compatibility
fallback staying available.

## Suggested Next

Continue Step 3 with Sema function/overload rendered-name lookup routes,
prioritizing helpers where structured metadata can make a miss authoritative
without relying on namespace-qualified rendered bridges.

## Watchouts

- Some existing parsed method-body positives still lack structured field
  `TextId` metadata, so instance-field lookup must keep rendered compatibility
  fallback when the structured field metadata chain is absent.
- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- This packet intentionally kept rendered overload lookup as compatibility
  fallback when the reference has no structured overload vector.
- This packet intentionally kept rendered consteval function lookup as
  compatibility fallback when the reference has neither structured nor `TextId`
  metadata.
- Consteval function lookup now treats a structured or `TextId` metadata miss
  as authoritative before consulting rendered-name compatibility; the retained
  fallback is only for references that genuinely lack those carriers.
- Parser `eval_const_int` callers that have named constants already use
  `std::unordered_map<TextId, long long>`; parser three-argument layout calls
  do not perform named-constant lookup.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.
- Namespace-qualified rendered global names such as using-import and anonymous
  namespace bridges still lack equivalent structured symbol metadata on this
  route, so this packet intentionally preserves those fallbacks.
- Synthetic locals without structured metadata, such as predefined function
  name identifiers, must remain rendered fallback candidates.

## Proof

Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval).*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 453/453 tests green. Proof log: `test_after.log`.
