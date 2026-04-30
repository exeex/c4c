# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 3 removed rendered-name authority from Sema ordinary function/ref-overload/
operator-overload lookup after structured function metadata makes a miss
authoritative. `lookup_function_by_name`, `lookup_ref_overloads_by_name`, and
`lookup_cpp_overloads_by_name` still prefer structured hits, but stale
unqualified rendered candidates with structured metadata no longer survive when
the reference's structured function key misses. Focused tests cover stale
rendered overload disagreement being ignored after structured misses and
rendered overload compatibility still being consulted when the reference has no
structured carrier.

## Suggested Next

Continue Step 3 with any remaining Sema rendered-name semantic lookup routes,
prioritizing routes that already have structured or `TextId` metadata and do
not rely on namespace-qualified rendered bridges.

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
- Ordinary function/ref-overload/operator-overload lookup now suppresses stale
  unqualified rendered candidates only when the rendered candidate was itself
  mirrored from structured metadata. No-metadata references and
  namespace-qualified rendered bridges remain compatibility fallbacks.

## Proof

Ran the delegated proof command:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|frontend_hir_lookup_tests|cpp_positive_sema_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*|cpp_negative_tests_.*(symbol|namespace|function|enum|member|method|static|call|consteval|overload).*)$' --output-on-failure) > test_after.log 2>&1`.
It passed with 464/464 tests green. Proof log: `test_after.log`.
